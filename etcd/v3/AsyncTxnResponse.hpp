#ifndef __ASYNC_TXNRESPONSE_HPP__
#define __ASYNC_TXNRESPONSE_HPP__

#include "proto/rpc.pb.h"
#include "etcd/v3/V3Response.hpp"

using etcdserverpb::TxnResponse;

namespace etcdv3
{
  class AsyncTxnResponse : public etcdv3::V3Response
  {
    public:
      AsyncTxnResponse(){};
      void ParseResponse(TxnResponse& resp);
      void ParseResponse(std::string const& key, bool prefix, TxnResponse& resp);
  };
}

#endif
