#ifndef __ASYNC_LEASETIMETOLIVERESPONSE_HPP__
#define __ASYNC_LEASETIMETOLIVERESPONSE_HPP__

#include <grpc++/grpc++.h>

#include "etcd/v3/V3Response.hpp"
#include "proto/rpc.grpc.pb.h"


using etcdserverpb::LeaseTimeToLiveResponse;

namespace etcdv3 {
class AsyncLeaseTimeToLiveResponse : public etcdv3::V3Response {
 public:
  AsyncLeaseTimeToLiveResponse(){};
  void ParseResponse(LeaseTimeToLiveResponse& resp);
};
}  // namespace etcdv3

#endif
