#ifndef __ASYNC_LEASELEASESRESPONSE_HPP__
#define __ASYNC_LEASELEASESRESPONSE_HPP__

#include <grpc++/grpc++.h>

#include "etcd/v3/V3Response.hpp"
#include "proto/rpc.grpc.pb.h"

using etcdserverpb::LeaseLeasesResponse;

namespace etcdv3 {
class AsyncLeaseLeasesResponse : public etcdv3::V3Response {
 public:
  AsyncLeaseLeasesResponse(){};
  void ParseResponse(LeaseLeasesResponse& resp);
};
}  // namespace etcdv3

#endif
