#ifndef __V3_ACTION_HPP__
#define __V3_ACTION_HPP__

#include <chrono>

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

namespace etcdv3
{
  enum AtomicityType
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
    std::string value;
    std::string old_value;
    std::string auth_token;
    KV::Stub* kv_stub;
    Watch::Stub* watch_stub;
    Lease::Stub* lease_stub;
    Lock::Stub* lock_stub;
    Election::Stub* election_stub;
  };

  class Action
  {
  public:
    Action(etcdv3::ActionParameters const &params);
    void waitForResponse();
    const std::chrono::high_resolution_clock::time_point startTimepoint();
  protected:
    Status status;
    ClientContext context;
    CompletionQueue cq_;
    etcdv3::ActionParameters parameters;
    std::chrono::high_resolution_clock::time_point start_timepoint;
  };

  namespace detail {
    std::string string_plus_one(std::string const &value);
  }
}
#endif
