#ifndef __ASYNC_LEASEKEEPALIVEACTION_HPP__
#define __ASYNC_LEASEKEEPALIVEACTION_HPP__

#include <grpc++/grpc++.h>

#include "etcd/Response.hpp"
#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncLeaseKeepAliveResponse.hpp"
#include "proto/rpc.grpc.pb.h"

using etcd::Response;
using etcdserverpb::LeaseKeepAliveRequest;
using etcdserverpb::LeaseKeepAliveResponse;
using grpc::ClientAsyncReaderWriter;
using grpc::ClientAsyncResponseReader;

namespace etcdv3 {
class AsyncLeaseKeepAliveAction : public etcdv3::Action {
  enum class Type { READ = 1, WRITE = 2, CONNECT = 3, WRITES_DONE = 4, FINISH = 5 };
 public:
  AsyncLeaseKeepAliveAction(etcdv3::ActionParameters param);
  AsyncLeaseKeepAliveResponse ParseResponse();
  void waitForResponse();

 private:
  LeaseKeepAliveResponse reply;
  std::unique_ptr<ClientAsyncReaderWriter<LeaseKeepAliveRequest, LeaseKeepAliveResponse>> stream;
};
}  // namespace etcdv3

#endif
