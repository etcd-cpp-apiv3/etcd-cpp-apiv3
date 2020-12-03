#include "etcd/KeepAlive.hpp"

#include <agents.h>
#include <ppl.h>

#include "etcd/v3/AsyncLeaseKeepAliveAction.hpp"
#include "etcd/v3/AsyncLeaseRevokeAction.hpp"

etcd::KeepAlive::KeepAlive(Client& client) : client_(client) { stub_ = etcdserverpb::Lease::NewStub(client.channel); }

pplx::task<void> etcd::KeepAlive::start(int refresh_in_ms) {
  pplx::task_completion_event<void> tce;

  // Start a repetetive timer that will trigger our lease keepalive refresh
  timer_ = std::unique_ptr<pplx::timer<int>>(new pplx::timer<int>(refresh_in_ms, 0, nullptr, true));

  // Open the stream
  stream_ = stub_->AsyncLeaseKeepAlive(&context_, &cq_, reinterpret_cast<void*>(Type::CONNECT));

  // Define the callback that is going to be repeatedly called to create a queue of lease id's
  // that need to be refreshed
  auto queueKeepAlivesCallback = new pplx::call<int>([this](int) {
    std::cout << " ** Queueing keepalives: " << std::endl;

    // Copy the lease id's to a queue for processing
    pplx::concurrent_unordered_map<int64_t, int>::iterator itr;
    for (itr = leases_.begin(); itr != leases_.end(); ++itr) {
      leaseQueue_.push(*itr);
    }

    // Kick off the process by sending the first keepalive command
    // You can have at most one write or at most one read at any given time,
    // so we are not sending another keepalive on the stream until the previous one
    // responds
    sendNextKeepAlive();
    // tce.set();
  });

  // Create the long running task that will monitor the Completion Queue and
  // send/receive data on the stream
  // On a CONNECT, this will kick off the timer which then triggers the keepalive's
  currentTask_ = pplx::task<void>([this, tce, queueKeepAlivesCallback]() {
    while (true) {
      void* got_tag;
      bool ok = false;
      // Block until the next result is available in the completion queue
      // "cq". The return value of Next should always be checked. This
      // return value tells us whether there is any kind of event or the cq_
      // is shutting down.
      std::cout << "Waiting for next event..." << std::endl;
      if (!cq_.Next(&got_tag, &ok)) {
        std::cerr << "Client stream closed. Quitting" << std::endl;
        break;
      }

      // It's important to process all tags even if the ok is false. One
      // might want to deallocate memory that has be reinterpret_cast'ed to
      // void* when the tag got initialized. For our example, we cast an int
      // to a void*, so we don't have extra memory management to take care
      // of.
      if (ok) {
        std::cout << std::endl << "**** Processing completion queue tag " << got_tag << std::endl;

        switch (static_cast<Type>(reinterpret_cast<int64_t>(got_tag))) {
          case Type::READ:
            std::cout << "Read a new message, sending next." << std::endl;
            sendNextKeepAlive();
            break;
          case Type::WRITE:
            std::cout << "Sent message (async), attempting to read response." << std::endl;
            readNextMessage();
            break;
          case Type::CONNECT:
            std::cout << "Server connected." << std::endl;
            tce.set();
            timer_->link_target(queueKeepAlivesCallback);
            timer_->start();
            break;
          case Type::WRITES_DONE:
            std::cout << "Server disconnecting." << std::endl;
            timer_->stop();
            break;
          case Type::FINISH:
            std::cout << "Client finish; status = " << (finish_status_.ok() ? "ok" : "cancelled") << std::endl;
            context_.TryCancel();
            cq_.Shutdown();
            break;
          default:
            std::cerr << "Unexpected tag " << got_tag << std::endl;
        }
      }
    }
  });

  // Returning a task that completes once the CONNECT completes, so that the
  // client can perform anything they need to at that time.
  pplx::task<void> start_completed(tce);
  return start_completed;
}

void etcd::KeepAlive::add(int64_t leaseid, int ttl) { leases_.insert(std::make_pair(leaseid, ttl)); }

void etcd::KeepAlive::remove(int64_t leaseid, bool revoke) {
  leases_.unsafe_erase(leaseid);
  if (revoke) {
    client_.leaserevoke(leaseid);
  }
}

void etcd::KeepAlive::sendNextKeepAlive() {
  std::pair<int64_t, int> lease;
  if (leaseQueue_.try_pop(lease)) {
    std::cout << "sending keepalive for " << lease.first << std::endl;
    LeaseKeepAliveRequest request;
    request.set_id(lease.first);
    stream_->Write(request, reinterpret_cast<void*>(Type::WRITE));
  } else {
    std::cout << "no keepalives left" << std::endl;
  }
}

void etcd::KeepAlive::readNextMessage() {
  std::cout << " ** Got response: " << response_.ttl() << std::endl;
  stream_->Read(&response_, reinterpret_cast<void*>(Type::READ));
}

etcd::KeepAlive::~KeepAlive() { cq_.Shutdown(); }
