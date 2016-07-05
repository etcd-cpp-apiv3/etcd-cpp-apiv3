#ifndef __ASYNC_WATCHACTION_HPP__
#define __ASYNC_WATCHACTION_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "v3/include/Action.hpp"
#include "v3/include/AsyncWatchResponse.hpp"
#include "etcd/Response.hpp"


using grpc::ClientAsyncReaderWriter;
using etcdserverpb::WatchRequest;
using etcdserverpb::WatchResponse;
using etcdserverpb::KV;
using etcdserverpb::Watch;

namespace etcdv3
{
  class AsyncWatchAction : public etcdv3::Action
  {
    public:
      AsyncWatchAction(std::string const & key, bool recursive, KV::Stub* stub_, Watch::Stub* watchServiceStub);
      AsyncWatchAction(std::string const & key, int fromIndex, bool recursive, KV::Stub* stub_, Watch::Stub* watchServiceStub);
      AsyncWatchResponse ParseResponse();
      void waitForResponse();
      void waitForResponse(std::function<void(etcd::Response)> callback); 
      void CancelWatch();
      void WatchReq(std::string const & key);
      WatchResponse reply;
      KV::Stub* stub_;
      std::unique_ptr<ClientAsyncReaderWriter<WatchRequest,WatchResponse>> stream;
      bool prefix;
    
  };
}

#endif
