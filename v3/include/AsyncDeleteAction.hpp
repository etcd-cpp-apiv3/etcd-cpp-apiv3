#ifndef __ASYNC_DELETE_HPP__
#define __ASYNC_DELETE_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "v3/include/Action.hpp"
#include "v3/include/AsyncDeleteRangeResponse.hpp"


using grpc::ClientAsyncResponseReader;
using etcdserverpb::DeleteRangeResponse;

namespace etcdv3
{
  class AsyncDeleteAction : public etcdv3::Action
  {
    public:
      AsyncDeleteAction(etcdv3::ActionParameters param);
      AsyncDeleteRangeResponse ParseResponse();
    private:
      DeleteRangeResponse reply;
      std::unique_ptr<ClientAsyncResponseReader<DeleteRangeResponse>> response_reader;
  };
}

#endif
