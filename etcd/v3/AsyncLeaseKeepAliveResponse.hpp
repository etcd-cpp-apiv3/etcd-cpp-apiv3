#ifndef __ASYNC_LEASEKEEPALIVERESPONSE_HPP__
#define __ASYNC_LEASEKEEPALIVERESPONSE_HPP__

#include <grpc++/grpc++.h>

#include "etcd/v3/V3Response.hpp"
#include "proto/rpc.grpc.pb.h"
#include "proto/rpc.pb.h"


using etcdserverpb::KV;
using etcdserverpb::LeaseKeepAliveRequest;
using etcdserverpb::LeaseKeepAliveResponse;

namespace etcdv3 {
class AsyncLeaseKeepAliveResponse : public etcdv3::V3Response {
 public:
  AsyncLeaseKeepAliveResponse(){};
  void ParseResponse(LeaseKeepAliveResponse& resp);
};
}  // namespace etcdv3

#endif
