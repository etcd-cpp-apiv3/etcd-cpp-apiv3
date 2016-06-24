#ifndef __ASYNC_RANGERESPONSE_HPP__
#define __ASYNC_RANGERESPONSE_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "v3/include/V3Response.hpp"


using grpc::ClientAsyncResponseReader;
using etcdserverpb::RangeResponse;

namespace etcdv3
{
  class AsyncRangeResponse : public etcdv3::V3Response
  {
    public:
      AsyncRangeResponse(RangeResponse& resp);
      AsyncRangeResponse(const AsyncRangeResponse& other);
      AsyncRangeResponse& operator=(const AsyncRangeResponse& other);
      void ParseResponse();
      RangeResponse reply;
  };
}

#endif
