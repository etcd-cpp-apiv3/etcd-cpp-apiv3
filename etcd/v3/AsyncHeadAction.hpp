#ifndef __ASYNC_HEAD_HPP__
#define __ASYNC_HEAD_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncHeadResponse.hpp"


using grpc::ClientAsyncResponseReader;
using etcdserverpb::RangeResponse;

namespace etcdv3
{
  class AsyncHeadAction : public etcdv3::Action
  {
    public:
      AsyncHeadAction(etcdv3::ActionParameters const &param);
      AsyncHeadResponse ParseResponse();
    private:
      RangeResponse reply;
      std::unique_ptr<ClientAsyncResponseReader<RangeResponse>> response_reader;
  };
}

#endif
