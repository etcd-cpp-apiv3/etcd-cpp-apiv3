#ifndef __ASYNC_PUTRESPONSE_HPP__
#define __ASYNC_PUTRESPONSE_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "v3/include/V3Response.hpp"
#include "v3/include/grpcClient.hpp"


using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using etcdserverpb::PutResponse;

namespace etcdv3
{
  class AsyncPutResponse : public etcdv3::V3Response
  {
    public:
      AsyncPutResponse(){};
      AsyncPutResponse(const std::string act){action = act;};
      AsyncPutResponse(const AsyncPutResponse& other);
      AsyncPutResponse& operator=(const AsyncPutResponse& other);
      PutResponse reply;
      Status status;
      ClientContext context;
      CompletionQueue cq_;
      std::unique_ptr<ClientAsyncResponseReader<PutResponse>> response_reader;
      AsyncPutResponse& ParseResponse();
      etcdv3::grpcClient* client;
      std::string key;
  };
}

#endif
