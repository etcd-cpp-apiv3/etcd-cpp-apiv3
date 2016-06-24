#ifndef __ASYNC_COMPAREANDSWAP_HPP__
#define __ASYNC_COMPAREANDSWAP_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "v3/include/Action.hpp"
#include "v3/include/AsyncTxnResponse.hpp"


using grpc::ClientAsyncResponseReader;
using etcdserverpb::TxnResponse;
using etcdserverpb::KV;

namespace etcdv3
{
  class AsyncCompareAndSwapAction : public etcdv3::Action
  {
    public:
      AsyncCompareAndSwapAction(std::string const & key, std::string const & value, std::string const & old_value, KV::Stub* stub_);
      AsyncCompareAndSwapAction(std::string const & key, std::string const & value, int old_index, KV::Stub* stub_);
      AsyncTxnResponse ParseResponse();
      TxnResponse reply;
      std::unique_ptr<ClientAsyncResponseReader<TxnResponse>> response_reader;
  };
}

#endif
