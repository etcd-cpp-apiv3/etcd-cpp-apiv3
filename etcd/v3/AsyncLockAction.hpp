#ifndef __ASYNC_LOCKACTION_HPP__
#define __ASYNC_LOCKACTION_HPP__

#include <grpc++/grpc++.h>
#include "proto/v3lock.grpc.pb.h"
#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncLockResponse.hpp"
#include "etcd/Response.hpp"


using grpc::ClientAsyncResponseReader;
using v3lockpb::LockRequest;
using v3lockpb::LockResponse;
using v3lockpb::UnlockRequest;
using v3lockpb::UnlockResponse;

namespace etcdv3
{
  class AsyncLockAction : public etcdv3::Action
  {
    public:
      AsyncLockAction(etcdv3::ActionParameters const &param);
      AsyncLockResponse ParseResponse();
    private:
      LockResponse reply;
      std::unique_ptr<ClientAsyncResponseReader<LockResponse>> response_reader;   
  };

  class AsyncUnlockAction : public etcdv3::Action
  {
    public:
      AsyncUnlockAction(etcdv3::ActionParameters const &param);
      AsyncUnlockResponse ParseResponse();
    private:
      UnlockResponse reply;
      std::unique_ptr<ClientAsyncResponseReader<UnlockResponse>> response_reader;   
  };
}

#endif
