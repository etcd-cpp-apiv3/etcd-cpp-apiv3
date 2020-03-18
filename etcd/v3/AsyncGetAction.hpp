#ifndef __ASYNC_GET_HPP__
#define __ASYNC_GET_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncRangeResponse.hpp"


using grpc::ClientAsyncResponseReader;
using etcdserverpb::RangeResponse;

namespace etcdv3
{
  class AsyncGetAction : public etcdv3::Action
  {
    public:
      AsyncGetAction(etcdv3::ActionParameters param);
      AsyncRangeResponse ParseResponse();
    private:
      RangeResponse reply;
      std::unique_ptr<ClientAsyncResponseReader<RangeResponse>> response_reader;
  };
}

#endif
