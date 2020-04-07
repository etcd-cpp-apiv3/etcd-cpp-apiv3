#ifndef __ASYNC_LOCK_HPP__
#define __ASYNC_LOCK_HPP__

#include <grpc++/grpc++.h>
#include "proto/v3lock.grpc.pb.h"
#include "v3/include/V3Response.hpp"


using grpc::ClientAsyncResponseReader;
using v3lockpb::LockRequest;
using v3lockpb::LockResponse;
using v3lockpb::UnlockRequest;
using v3lockpb::UnlockResponse;

namespace etcdv3
{
  class AsyncLockResponse : public etcdv3::V3Response
  {
    public:
      AsyncLockResponse(){};
      void ParseResponse(LockResponse& resp);
  };

  class AsyncUnlockResponse : public etcdv3::V3Response
  {
    public:
      AsyncUnlockResponse(){};
      void ParseResponse(UnlockResponse& resp);
  };
}

#endif

