#ifndef __ASYNC_HEADRESPONSE_HPP__
#define __ASYNC_HEADRESPONSE_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "etcd/v3/V3Response.hpp"


using grpc::ClientAsyncResponseReader;
using etcdserverpb::RangeResponse;

namespace etcdv3
{
  class AsyncHeadResponse : public etcdv3::V3Response
  {
    public:
      AsyncHeadResponse(){};
      void ParseResponse(RangeResponse& resp);
  };
}

#endif
