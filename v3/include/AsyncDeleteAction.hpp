#ifndef __ASYNC_DELETE_HPP__
#define __ASYNC_DELETE_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "v3/include/Action.hpp"
#include "v3/include/AsyncTxnResponse.hpp"


using grpc::ClientAsyncResponseReader;
using etcdserverpb::TxnResponse;
using etcdserverpb::KV;

namespace etcdv3
{
  class AsyncDeleteAction : public etcdv3::Action
  {
    public:
      AsyncDeleteAction(std::string const & key, KV::Stub* stub_, bool recursive=false);
      AsyncTxnResponse ParseResponse();
      TxnResponse reply;
      std::unique_ptr<ClientAsyncResponseReader<TxnResponse>> response_reader;
  };
}

#endif
