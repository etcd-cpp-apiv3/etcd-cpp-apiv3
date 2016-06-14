#ifndef __ASYNC_RANGERESPONSE_HPP__
#define __ASYNC_RANGERESPONSE_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "v3/include/V3Response.hpp"


using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using etcdserverpb::RangeResponse;

namespace etcdv3
{
  class AsyncRangeResponse : public etcdv3::V3Response
  {
    public:
      AsyncRangeResponse(){action = "get";};
      AsyncRangeResponse(const AsyncRangeResponse& other);
      AsyncRangeResponse& operator=(const AsyncRangeResponse& other);
      RangeResponse reply;
      etcdserverpb::PutResponse r;
      Status status;
      ClientContext context;
      CompletionQueue cq_;
      std::unique_ptr<ClientAsyncResponseReader<RangeResponse>> response_reader;
      AsyncRangeResponse& ParseResponse();
  };
}

#endif
