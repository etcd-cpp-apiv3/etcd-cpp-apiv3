#pragma once

#include <agents.h>
#include <concurrent_unordered_map.h>
#include <grpc++/grpc++.h>

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
class KeepAlive {
  enum class Type { READ = 1, WRITE = 2, CONNECT = 3, WRITES_DONE = 4, FINISH = 5 };

 public:
  /**
   * Create an instance of the KeepAlive service
   * Call start() to being the timer that automatically sends keepalives
   * Call add(leaseid) to add a leaseid to be kept alive by this service
   * @param client the client etcd connection to use
   */
  KeepAlive(Client& client);

  /**
   * Start processing keepalive's
   * The refresh time must be less than the granted TTL of your
   * leases, otherwise they will expire. Create multiple KeepAlive
   * services with different refreshes if you have many leases with varied
   * TTL's
   * @param refresh_in_ms the time between refreshes (default: 5000ms)
   */
  pplx::task<void> start(int refresh_in_ms = 5000);

  /**
   * Add a lease to be kept alive by this service
   * @param leaseid the id of the lease
   * @param ttl the ttl of the lease (in seconds) <reserved for future use>
   */
  void add(int64_t leaseid, int ttl = 5);

  /**
   * Remove a lease from being kept alive, and optionally revoke it immediately
   * @param leaseid the id of the lease
   * @param revoke true to immediatley revoke the lease (defaults to false)
   */
  void remove(int64_t leaseid, bool revoke = false);

  ~KeepAlive();

 private:
  /**
   * Sends a keep alive for the next lease in the queue
   */
  void sendNextKeepAlive();
  /**
   *
   */
  void readNextMessage();

  // The timer that is triggering refreshes
  std::unique_ptr<pplx::timer<int>> timer_;

  // The map of leases that have been registered with this service to be kept alive
  pplx::concurrent_unordered_map<int64_t, int> leases_;

  // The current queue of leases that still need to be refreshed on this pass of the timer
  pplx::concurrent_queue<std::pair<int64_t, int>> leaseQueue_;

  // The long running task for this service
  pplx::task<void> currentTask_;

  // The client that we are attached to.
  Client& client_;

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
      grpc::ClientAsyncReaderWriter<etcdserverpb::LeaseKeepAliveRequest, etcdserverpb::LeaseKeepAliveResponse>>
      stream_;

  // Allocated protobuf that holds the response. In real clients and servers,
  // the memory management would a bit more complex as the thread that fills
  // in the response should take care of concurrency as well as memory
  // management.
  etcdserverpb::LeaseKeepAliveResponse response_;

  // Finish status when the client is done with the stream.
  grpc::Status finish_status_ = grpc::Status::OK;
};
}  // namespace etcd
