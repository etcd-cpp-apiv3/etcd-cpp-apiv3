#ifndef __ASYNC_DELETERESPONSE_HPP__
#define __ASYNC_DELETERESPONSE_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "etcd/v3/V3Response.hpp"
#include "etcd/v3/Action.hpp"


using grpc::ClientAsyncResponseReader;
using etcdserverpb::DeleteRangeResponse;

namespace etcdv3
{
  class AsyncDeleteRangeResponse : public etcdv3::V3Response
  {
    public:
      AsyncDeleteRangeResponse(){};
      void ParseResponse(std::string const& key, bool prefix, DeleteRangeResponse& resp);
  };
}

#endif
