#include <chrono>

#include "etcd/KeepAlive.hpp"
#include "etcd/v3/AsyncLeaseAction.hpp"

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"

namespace etcdv3 {
  class AsyncLeaseKeepAliveAction;
}

struct etcd::KeepAlive::EtcdServerStubs {
  std::unique_ptr<etcdserverpb::Lease::Stub> leaseServiceStub;
  std::unique_ptr<etcdv3::AsyncLeaseKeepAliveAction> call;
};

void etcd::KeepAlive::EtcdServerStubsDeleter::operator()(etcd::KeepAlive::EtcdServerStubs *stubs) {
  if (stubs) {
    delete stubs;
  }
}

etcd::KeepAlive::KeepAlive(Client const &client, int ttl, int64_t lease_id):
    ttl(ttl), lease_id(lease_id), continue_next(true) {
  stubs.reset(new EtcdServerStubs{});
  stubs->leaseServiceStub = Lease::NewStub(client.channel);

  etcdv3::ActionParameters params;
  params.auth_token.assign(client.current_auth_token());
  params.lease_id = this->lease_id;
  params.lease_stub = stubs->leaseServiceStub.get();

  continue_next.store(true);

  stubs->call.reset(new etcdv3::AsyncLeaseKeepAliveAction(params));
  task_ = std::thread([this]() {
    try {
      // start refresh
      this->refresh();
      context.run();
    } catch (...) {
      eptr_ = std::current_exception();
    }
    context.stop();  // clean up
  });
}

etcd::KeepAlive::KeepAlive(std::string const & address, int ttl, int64_t lease_id):
    KeepAlive(Client(address), ttl, lease_id) {
}

etcd::KeepAlive::KeepAlive(std::string const & address,
                           std::string const & username, std::string const & password,
                           int ttl, int64_t lease_id, int const auth_token_ttl):
    KeepAlive(Client(address, username, password, auth_token_ttl), ttl, lease_id) {
}

etcd::KeepAlive::KeepAlive(Client const &client,
                           std::function<void (std::exception_ptr)> const &handler,
                           int ttl, int64_t lease_id):
    handler_(handler), ttl(ttl), lease_id(lease_id), continue_next(true) {
  stubs.reset(new EtcdServerStubs{});
  stubs->leaseServiceStub = Lease::NewStub(client.channel);

  etcdv3::ActionParameters params;
  params.auth_token.assign(client.current_auth_token());
  params.lease_id = this->lease_id;
  params.lease_stub = stubs->leaseServiceStub.get();

  stubs->call.reset(new etcdv3::AsyncLeaseKeepAliveAction(params));
  task_ = std::thread([this]() {
    try {
      // start refresh
      this->refresh();
      context.run();
    } catch (...) {
      if (handler_) {
        handler_(std::current_exception());
      } else {
        eptr_ = std::current_exception();
      }
      this->Cancel();
    }
  });
}

etcd::KeepAlive::KeepAlive(std::string const & address,
                           std::function<void (std::exception_ptr)> const &handler,
                           int ttl, int64_t lease_id):
    KeepAlive(Client(address), handler, ttl, lease_id) {
}

etcd::KeepAlive::KeepAlive(std::string const & address,
                           std::string const & username, std::string const & password,
                           std::function<void (std::exception_ptr)> const &handler,
                           int ttl, int64_t lease_id, const int auth_token_ttl):
    KeepAlive(Client(address, username, password, auth_token_ttl), handler, ttl, lease_id) {
}

etcd::KeepAlive::~KeepAlive()
{
  this->Cancel();
  // clean up
  if (task_.joinable()) {
    task_.join();
  }
}

void etcd::KeepAlive::Cancel()
{
  if (!continue_next.exchange(false)) {
    return;
  }
  stubs->call->CancelKeepAlive();
  if (keepalive_timer_) {
    keepalive_timer_->cancel();
  }
  context.stop();
}

void etcd::KeepAlive::Check() {
  if (eptr_) {
    std::rethrow_exception(eptr_);
  }
}

void etcd::KeepAlive::refresh()
{
  if (!continue_next.load()) {
    return;
  }
  // minimal resolution: 1 second
  int keepalive_ttl = std::max(ttl - 1, 1);
  keepalive_timer_.reset(new boost::asio::steady_timer(
      context, std::chrono::seconds(keepalive_ttl)));
  keepalive_timer_->async_wait([this](const boost::system::error_code& error) {
    if (error) {
#ifndef NDEBUG
      std::cerr << "keepalive timer cancelled: " << error << ", " << error.message() << std::endl;
#endif
    } else {
      if (this->continue_next.load()) {
        auto resp = this->stubs->call->Refresh();
        if (!resp.is_ok()) {
          throw std::runtime_error("Failed to refresh lease: error code: " + std::to_string(resp.error_code()) +
                                   ", message: " + resp.error_message());
        }
        if (resp.value().ttl() == 0) {
          throw std::out_of_range("Failed to refresh lease due to expiration: the new TTL is 0.");
        }
        // trigger the next round;
        this->refresh();
      }
    }
  });
}
