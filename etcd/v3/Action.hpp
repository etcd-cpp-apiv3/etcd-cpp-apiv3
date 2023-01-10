#ifndef __V3_ACTION_HPP__
#define __V3_ACTION_HPP__

#include <chrono>
#include <ostream>

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "proto/v3lock.grpc.pb.h"
#include "proto/v3election.grpc.pb.h"

using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

using etcdserverpb::KV;
using etcdserverpb::Watch;
using etcdserverpb::Lease;
using v3lockpb::Lock;
using v3electionpb::Election;

namespace etcd {
  class Response;
}

namespace etcdv3
{
  enum class AtomicityType
  {
    PREV_INDEX = 0,
    PREV_VALUE = 1
  };

  struct ActionParameters
  {
    ActionParameters();
    bool withPrefix;
    int64_t revision;
    int64_t old_revision;
    int64_t lease_id = 0;  // no lease
    int ttl;
    int limit;
    std::string name;  // for campaign (in v3election)
    std::string key;
    std::string range_end;
    bool keys_only;
    bool count_only;
    std::string value;
    std::string old_value;
    std::string auth_token;
    std::chrono::microseconds grpc_timeout = std::chrono::microseconds::zero();
    KV::Stub* kv_stub;
    Watch::Stub* watch_stub;
    Lease::Stub* lease_stub;
    Lock::Stub* lock_stub;
    Election::Stub* election_stub;

    bool has_grpc_timeout() const;
    std::chrono::system_clock::time_point grpc_deadline() const;

    void dump(std::ostream &os) const;
  };

  class Action
  {
  public:
    Action(etcdv3::ActionParameters const &params);
    Action(etcdv3::ActionParameters && params);
    void waitForResponse();
    const std::chrono::high_resolution_clock::time_point startTimepoint();
  protected:
    Status status;
    ClientContext context;
    CompletionQueue cq_;
    etcdv3::ActionParameters parameters;
    std::chrono::high_resolution_clock::time_point start_timepoint;
  private:
    // Init things like auth token, etc.
    void InitAction();

    friend class etcd::Response;
  };

  namespace detail {
    std::string string_plus_one(std::string const &value);
  }
}
#endif
