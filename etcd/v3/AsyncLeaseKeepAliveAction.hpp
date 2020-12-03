#ifndef __ASYNC_LEASEKEEPALIVEACTION_HPP__
#define __ASYNC_LEASEKEEPALIVEACTION_HPP__

#include <grpc++/grpc++.h>

#include "etcd/Response.hpp"
#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncleaseKeepAliveResponse.hpp"
#include "proto/rpc.grpc.pb.h"

using etcd::Response;
using etcdserverpb::LeaseKeepAliveRequest;
using etcdserverpb::LeaseKeepAliveResponse;
using grpc::ClientAsyncReaderWriter;
using grpc::ClientAsyncResponseReader;

namespace etcdv3 {
class AsyncLeaseKeepAliveAction : public etcdv3::Action {
 public:
  AsyncLeaseKeepAliveAction(etcdv3::ActionParameters param);
  AsyncLeaseKeepAliveResponse ParseResponse();
  void waitForResponse();

 private:
  LeaseKeepAliveResponse reply;
  std::unique_ptr<ClientAsyncReaderWriter<LeaseKeepAliveRequest, LeaseKeepAliveResponse>> stream;

  char* doneTag = "writes done";
};
}  // namespace etcdv3

#endif
