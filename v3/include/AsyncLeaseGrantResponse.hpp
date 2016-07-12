#ifndef __ASYNC_LEASEGRANTRESPONSE_HPP__
#define __ASYNC_LEASEGRANTRESPONSE_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "v3/include/V3Response.hpp"


using etcdserverpb::LeaseGrantResponse;

namespace etcdv3
{
  class AsyncLeaseGrantResponse : public etcdv3::V3Response
  {
    public:
      AsyncLeaseGrantResponse(){};
      void ParseResponse(LeaseGrantResponse& resp);
  };
}

#endif
