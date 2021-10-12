#ifndef __ASYNC_PUTRESPONSE_HPP__
#define __ASYNC_PUTRESPONSE_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "etcd/v3/V3Response.hpp"
#include "etcd/v3/Action.hpp"


using grpc::ClientAsyncResponseReader;
using etcdserverpb::PutResponse;

namespace etcdv3
{
  class AsyncPutResponse : public etcdv3::V3Response
  {
    public:
      AsyncPutResponse(){};
      void ParseResponse(PutResponse& resp);
  };
}

#endif
