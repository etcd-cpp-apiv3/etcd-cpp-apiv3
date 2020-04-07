#ifndef __ASYNC_TXNACTION_HPP__
#define __ASYNC_TXNACTION_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "v3/include/Action.hpp"
#include "v3/include/AsyncTxnResponse.hpp"
#include "v3/include/Transaction.hpp"


using grpc::ClientAsyncResponseReader;
using etcdserverpb::TxnResponse;
using etcdserverpb::KV;

namespace etcdv3
{
  class AsyncTxnAction : public etcdv3::Action
  {
    public:
      AsyncTxnAction(etcdv3::ActionParameters param, etcdv3::Transaction const &tx);
      AsyncTxnResponse ParseResponse();
    private:
      TxnResponse reply;
      std::unique_ptr<ClientAsyncResponseReader<TxnResponse>> response_reader;
  };
}

#endif
