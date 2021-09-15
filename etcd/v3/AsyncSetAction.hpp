#ifndef __ASYNC_SET_HPP__
#define __ASYNC_SET_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncTxnResponse.hpp"


using grpc::ClientAsyncResponseReader;
using etcdserverpb::TxnResponse;
using etcdserverpb::KV;

namespace etcdv3
{
  class AsyncSetAction : public etcdv3::Action
  {
    public:
      AsyncSetAction(etcdv3::ActionParameters const &param, bool create=false);
      AsyncTxnResponse ParseResponse();
    private:
      TxnResponse reply;
      std::unique_ptr<ClientAsyncResponseReader<TxnResponse>> response_reader;
      bool isCreate;
  };
}

#endif
