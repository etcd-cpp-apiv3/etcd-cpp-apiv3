#ifndef __ASYNC_LEASEREVOKERESPONSE_HPP__
#define __ASYNC_LEASEREVOKERESPONSE_HPP__

#include <grpc++/grpc++.h>

#include "etcd/v3/V3Response.hpp"
#include "proto/rpc.grpc.pb.h"


using etcdserverpb::LeaseRevokeResponse;

namespace etcdv3 {
class AsyncLeaseRevokeResponse : public etcdv3::V3Response {
 public:
  AsyncLeaseRevokeResponse(){};
  void ParseResponse(LeaseRevokeResponse& resp);
};
}  // namespace etcdv3

#endif
