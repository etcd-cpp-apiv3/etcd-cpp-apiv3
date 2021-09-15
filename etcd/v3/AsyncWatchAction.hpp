#ifndef __ASYNC_WATCHACTION_HPP__
#define __ASYNC_WATCHACTION_HPP__

#include <atomic>
#include <mutex>

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncWatchResponse.hpp"
#include "etcd/Response.hpp"

using grpc::ClientAsyncReaderWriter;
using etcdserverpb::WatchRequest;
using etcdserverpb::WatchResponse;

namespace etcdv3
{
  class AsyncWatchAction : public etcdv3::Action
  {
    public:
      AsyncWatchAction(etcdv3::ActionParameters const &param);
      AsyncWatchResponse ParseResponse();
      void waitForResponse();
      void waitForResponse(std::function<void(etcd::Response)> callback); 
      void CancelWatch();
      bool Cancelled() const;
    private:
      WatchResponse reply;
      std::unique_ptr<ClientAsyncReaderWriter<WatchRequest,WatchResponse>> stream;   
      std::atomic_bool isCancelled;
      std::mutex protect_is_cancalled;
  };
}

#endif
