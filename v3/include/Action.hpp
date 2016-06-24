#ifndef __V3_ACTION_HPP__
#define __V3_ACTION_HPP__

#include <grpc++/grpc++.h>

using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

namespace etcdv3
{
  class Action
  {
    public:
    Status status;
    ClientContext context;
    CompletionQueue cq_;
    void waitForResponse();
  };
}
#endif
