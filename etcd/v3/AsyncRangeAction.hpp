#ifndef __ASYNC_RANGE_HPP__
#define __ASYNC_RANGE_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncRangeResponse.hpp"


using grpc::ClientAsyncResponseReader;
using etcdserverpb::RangeResponse;

namespace etcdv3
{
  class AsyncRangeAction : public etcdv3::Action
  {
    public:
      AsyncRangeAction(etcdv3::ActionParameters const &param);
      AsyncRangeResponse ParseResponse();
    private:
      RangeResponse reply;
      std::unique_ptr<ClientAsyncResponseReader<RangeResponse>> response_reader;
  };
}

#endif
