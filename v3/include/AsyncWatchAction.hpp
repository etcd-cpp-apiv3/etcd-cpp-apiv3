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


namespace etcdv3
{
  class AsyncWatchAction : public etcdv3::Action
  {
    public:
      AsyncWatchAction(etcdv3::ActionParameters param);
      AsyncWatchResponse ParseResponse();
      void waitForResponse();
      void waitForResponse(std::function<void(etcd::Response)> callback); 
      void CancelWatch();
      void WatchReq(std::string const & key);
    private:
      WatchResponse reply;
      std::unique_ptr<ClientAsyncReaderWriter<WatchRequest,WatchResponse>> stream;   
      bool isCancelled;
  };
}

#endif
