#ifndef __ASYNC_WATCH_HPP__
#define __ASYNC_WATCH_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "v3/include/V3Response.hpp"


using grpc::ClientAsyncReaderWriter;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using etcdserverpb::WatchRequest;
using etcdserverpb::WatchResponse;
using etcdserverpb::KV;

namespace etcdv3
{
  class AsyncWatchResponse : public etcdv3::V3Response
  {
    public:
      AsyncWatchResponse(){fromIndex = -1;};
      AsyncWatchResponse(const std::string act){action = act;};
      AsyncWatchResponse(const AsyncWatchResponse& other);
      AsyncWatchResponse& operator=(const AsyncWatchResponse& other);
      AsyncWatchResponse& ParseResponse();
      void waitForResponse();
      WatchResponse reply;
      Status status;
      ClientContext context;
      CompletionQueue cq_;
      std::unique_ptr<ClientAsyncReaderWriter<WatchRequest,WatchResponse>> stream;
      KV::Stub* stub_;
      int fromIndex;
  };
}

#endif
