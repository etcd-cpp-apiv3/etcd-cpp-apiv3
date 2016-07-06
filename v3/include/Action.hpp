#ifndef __V3_ACTION_HPP__
#define __V3_ACTION_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"

using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

using etcdserverpb::KV;
using etcdserverpb::Watch;

namespace etcdv3
{
  enum Atomicity_Type
  {
    PREV_INDEX = 0,
    PREV_VALUE = 1
  };

  struct ActionParameters
  {
    bool withPrefix;
    int revision;
    int old_revision;
    std::string key;
    std::string value;
    std::string old_value;
    KV::Stub* kv_stub;
    Watch::Stub* watch_stub;
  };

  class Action
  {
    public:
    Status status;
    ClientContext context;
    CompletionQueue cq_;
    void waitForResponse();
  };

  class Actionv2
  {
    public:
    Actionv2(etcdv3::ActionParameters params);
    Status status;
    ClientContext context;
    CompletionQueue cq_;
    etcdv3::ActionParameters parameters;
    void waitForResponse();
  };
}
#endif
