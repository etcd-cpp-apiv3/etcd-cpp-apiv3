#ifndef __V3_ACTION_HPP__
#define __V3_ACTION_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"

using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

using etcdserverpb::KV;
using etcdserverpb::Watch;
using etcdserverpb::Lease;

namespace etcdv3
{
  enum Atomicity_Type
  {
    PREV_INDEX = 0,
    PREV_VALUE = 1
  };

  struct ActionParameters
  {
    ActionParameters();
    bool withPrefix;
    int revision;
    int old_revision;
    int64_t lease_id;
    int ttl;
    std::string key;
    std::string value;
    std::string old_value;
    KV::Stub* kv_stub;
    Watch::Stub* watch_stub;
    Lease::Stub* lease_stub;
  };

  class Action
  {
  public:
    Action(etcdv3::ActionParameters params);
    Action(){};
    void waitForResponse();
  protected:
    Status status;
    ClientContext context;
    CompletionQueue cq_;
    etcdv3::ActionParameters parameters;
    
  };
}
#endif
