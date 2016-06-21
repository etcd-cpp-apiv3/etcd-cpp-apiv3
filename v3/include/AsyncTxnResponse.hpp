#ifndef __ASYNC_TXNRESPONSE_HPP__
#define __ASYNC_TXNRESPONSE_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "v3/include/V3Response.hpp"

using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using etcdserverpb::TxnResponse;

namespace etcdv3
{
  class AsyncTxnResponse : public etcdv3::V3Response
  {
    public:
      AsyncTxnResponse(){};
      AsyncTxnResponse(const std::string act){action = act;};
      AsyncTxnResponse(const AsyncTxnResponse& other);
      AsyncTxnResponse& operator=(const AsyncTxnResponse& other);
      AsyncTxnResponse& ParseResponse();
      void waitForResponse();
      TxnResponse reply;
      Status status;
      ClientContext context;
      CompletionQueue cq_;
      std::unique_ptr<ClientAsyncResponseReader<TxnResponse>> response_reader;
  };
}

#endif
