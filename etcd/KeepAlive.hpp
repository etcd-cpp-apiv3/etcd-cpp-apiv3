#pragma once

#include <grpc++/grpc++.h>

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/thread.hpp>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <string>

#include "etcd/Client.hpp"
#include "etcd/Response.hpp"
#include "proto/rpc.grpc.pb.h"

namespace etcdv3 {
class AsyncKeepAliveAction;
}

using etcdserverpb::KV;
using grpc::Channel;

namespace etcd {
/**
 * Wrapper for a boost::asio timer that provides simple start/stop/repeat
 * functions
 */
class KeepAliveTimer {
public:
  typedef boost::function<void(boost::system::error_code, KeepAliveTimer *)>
      handler_function;

  KeepAliveTimer(boost::asio::io_context &io,
                 boost::asio::chrono::milliseconds interval)
      : interval_(interval), timer_(boost::asio::steady_timer(io, interval)) {}
  ~KeepAliveTimer() { timer_.cancel(); }

  void setCallback(handler_function handler) { handler_ = handler; }

  void start() {
    timer_.expires_from_now(boost::asio::chrono::milliseconds(
        50) /* expire immediately; set to interval_ otherwise*/);
    timer_.async_wait(boost::bind(handler_, boost::asio::placeholders::error,
                                  this)); // boost::ref(*this)));
  }

  void repeat() {
    timer_.expires_at(timer_.expiry() + interval_);
    timer_.async_wait(boost::bind(handler_, boost::asio::placeholders::error,
                                  this)); // boost::ref(*this)));
  }

  void stop() { timer_.cancel(); }

private:
  boost::asio::chrono::milliseconds interval_;
  boost::asio::steady_timer timer_;  
  handler_function handler_;
};

/**
 * The main KeepAlive service class. You only need one per process since it can
 * service many leases with different refresh intervals
 */
class KeepAlive {
  enum class Type {
    READ = 1,
    WRITE = 2,
    CONNECT = 3,
    WRITES_DONE = 4,
    FINISH = 5
  };

public:
  /**
   * Create an instance of the KeepAlive service
   * Call add(leaseid, refresh_interval) to add a leaseid to be kept alive by
   * this service
   * @param client the client etcd connection to use
   */
  KeepAlive(Client &client);

  /**
   * Add a lease to be kept alive by this service
   * @param leaseid the id of the lease
   * @param refresh_interval the interval to refresh the lease
   */
  void add(int64_t leaseid, boost::asio::chrono::milliseconds refresh_interval);

  /**
   * Remove a lease from being kept alive, and optionally revoke it immediately
   * @param leaseid the id of the lease
   * @param revoke true to immediatley revoke the lease (defaults to false)
   */
  void remove(int64_t leaseid, bool revoke = false);

  /**
   * Remove all leases that the service is currently refreshing.
   * @param revoke true to immediately revoke the leases (defaulst to false)
   */
  void removeAll(bool revoke = false);

  ~KeepAlive();

private:
  KeepAlive(KeepAlive const &rhs);            // prevent copying
  KeepAlive &operator=(KeepAlive const &rhs); // prevent assignment

  /**
   * Moves the registered leases to a processing queue, and then resets the
   * timer and triggers the first lease renewal request
   */
  void queueKeepAlivesCallback(const boost::system::error_code &error,
                               KeepAliveTimer *timer);

  /**
   * Sends a keep alive for the next lease in the queue
   */
  void sendNextKeepAlive(std::unique_ptr<grpc::ClientAsyncReaderWriter<
                             etcdserverpb::LeaseKeepAliveRequest,
                             etcdserverpb::LeaseKeepAliveResponse>> &stream);

  /**
   * Read the response off the stream
   */
  void readNextMessage(std::unique_ptr<grpc::ClientAsyncReaderWriter<
                           etcdserverpb::LeaseKeepAliveRequest,
                           etcdserverpb::LeaseKeepAliveResponse>> &stream);

  // Boost io_context, which is responsible for doing the timer work
  boost::asio::io_context io_;
  // Thread for the io_context to use
  std::unique_ptr<boost::thread> t_;

  // A mutex to use to protect all of our lease maps/queues when manipulating
  // them
  std::mutex mutex_;
  // The map of leases that have been registered with this service to be kept
  // alive
  std::map<int64_t, std::shared_ptr<KeepAliveTimer>> leases_;
  // All of the timers that are currently running
  std::map<boost::asio::chrono::milliseconds, std::shared_ptr<KeepAliveTimer>>
      timers_;
  // The current queue of keepalives that need to be sent
  std::queue<int64_t> keepalive_queue_;

  // The long running task for this service
  pplx::task<void> current_task_;

  // The client that we are attached to.
  Client &client_;

  // Context for the client. It could be used to convey extra information to
  // the server and/or tweak certain RPC behaviors.
  grpc::ClientContext context_;

  // The producer-consumer queue we use to communicate asynchronously with the
  // gRPC runtime.
  grpc::CompletionQueue cq_;

  // Out of the passed in Channel comes the stub, stored here, our view of the
  // server's exposed services.
  std::unique_ptr<etcdserverpb::Lease::Stub> stub_;

  // The bidirectional, asynchronous stream for sending/receiving messages.
  std::unique_ptr<
      grpc::ClientAsyncReaderWriter<etcdserverpb::LeaseKeepAliveRequest,
                                    etcdserverpb::LeaseKeepAliveResponse>>
      stream_;

  // Allocated protobuf that holds the response. In real clients and servers,
  // the memory management would a bit more complex as the thread that fills
  // in the response should take care of concurrency as well as memory
  // management.
  etcdserverpb::LeaseKeepAliveResponse response_;

  // Finish status when the client is done with the stream.
  grpc::Status finish_status_ = grpc::Status::OK;
};
} // namespace etcd
