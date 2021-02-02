#include <chrono>

#include "etcd/KeepAlive.hpp"
#include "etcd/v3/AsyncLeaseAction.hpp"

etcd::KeepAlive::KeepAlive(Client const &client, int ttl, int64_t lease_id):
    ttl(ttl), lease_id(lease_id), continue_next(true) {
  leaseServiceStub= Lease::NewStub(client.channel);

  etcdv3::ActionParameters params;
  params.auth_token.assign(client.auth_token);
  params.lease_id = this->lease_id;
  params.lease_stub = leaseServiceStub.get();

  call.reset(new etcdv3::AsyncLeaseKeepAliveAction(params));
  currentTask = pplx::task<void>([this]() {
    // start refresh
    this->refresh();
    context.run();
    context.stop();  // clean up
  });
}

etcd::KeepAlive::KeepAlive(std::string const & address, int ttl, int64_t lease_id):
    KeepAlive(Client(address), ttl, lease_id) {
}

etcd::KeepAlive::KeepAlive(std::string const & address,
                           std::string const & username, std::string const & password,
                           int ttl, int64_t lease_id):
    KeepAlive(Client(address, username, password), ttl, lease_id) {
}

etcd::KeepAlive::~KeepAlive()
{
  this->Cancel();
}

void etcd::KeepAlive::Cancel()
{
  if (!continue_next) {
    return;
  }
  continue_next = false;
#ifndef NDEBUG
  {
    std::ios::fmtflags os_flags (std::cout.flags());
    std::cout << "Cancel keepalive for " << std::hex << lease_id << std::endl;
    std::cout.flags(os_flags);
  }
#endif
  call->CancelKeepAlive();
  if (keepalive_timer_) {
    keepalive_timer_->cancel();
  }
  currentTask.wait();
}

void etcd::KeepAlive::refresh()
{
  if (!continue_next) {
    return;
  }
  // minimal resolution: 1 second
  int keepalive_ttl = std::max(ttl - 1, 1);
#ifndef NDEBUG
  {
    std::ios::fmtflags os_flags (std::cout.flags());
    std::cout << "Trigger the next keepalive round with ttl " << keepalive_ttl
              << " for " << std::hex << lease_id << std::endl;
    std::cout.flags(os_flags);
  }
#endif
  keepalive_timer_.reset(new boost::asio::steady_timer(
      context, std::chrono::seconds(keepalive_ttl)));
  keepalive_timer_->async_wait([this](const boost::system::error_code& error) {
    if (error) {
#ifndef NDEBUG
      std::cerr << "keepalive timer error: " << error << ", " << error.message() << std::endl;
#endif
    } else {
      this->call->Refresh();
      // trigger the next round;
      this->refresh();
    }
  });
}
