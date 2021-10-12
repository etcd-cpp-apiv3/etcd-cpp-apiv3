#ifndef __ASYNC_PUT_HPP__
#define __ASYNC_PUT_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncPutResponse.hpp"


using grpc::ClientAsyncResponseReader;
using etcdserverpb::PutResponse;

namespace etcdv3
{
  class AsyncPutAction : public etcdv3::Action
  {
    public:
      AsyncPutAction(etcdv3::ActionParameters const &param);
      AsyncPutResponse ParseResponse();
    private:
      PutResponse reply;
      std::unique_ptr<ClientAsyncResponseReader<PutResponse>> response_reader;
  };
}

#endif
