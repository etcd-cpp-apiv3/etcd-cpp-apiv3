#ifndef __ASYNC_DELETE_HPP__
#define __ASYNC_DELETE_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncDeleteResponse.hpp"


using grpc::ClientAsyncResponseReader;
using etcdserverpb::DeleteRangeResponse;

namespace etcdv3
{
  class AsyncDeleteAction : public etcdv3::Action
  {
    public:
      AsyncDeleteAction(etcdv3::ActionParameters const &param);
      AsyncDeleteResponse ParseResponse();
    private:
      DeleteRangeResponse reply;
      std::unique_ptr<ClientAsyncResponseReader<DeleteRangeResponse>> response_reader;
  };
}

#endif
