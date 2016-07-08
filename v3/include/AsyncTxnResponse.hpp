#ifndef __ASYNC_TXNRESPONSE_HPP__
#define __ASYNC_TXNRESPONSE_HPP__

#include "v3/include/V3Response.hpp"
#include "proto/rpc.pb.h"

using etcdserverpb::TxnResponse;

namespace etcdv3
{
  class AsyncTxnResponse : public etcdv3::V3Response
  {
    public:
      AsyncTxnResponse(){};
      void ParseResponse(std::string const& key, bool prefix,TxnResponse& resp);
  };
}

#endif
