#ifndef __ETCD_WATCHER_HPP__
#define __ETCD_WATCHER_HPP__

#include <string>

#include "etcd/Client.hpp"
#include "etcd/Response.hpp"

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"

namespace etcdv3 {
  class AsyncWatchAction;
}

using etcdserverpb::KV;
using etcdserverpb::Watch;
using grpc::Channel;

namespace etcd
{
  class Watcher
  {
  public:
    Watcher(Client &client, std::string const & key,
            std::function<void(Response)> callback, bool recursive=false);
    Watcher(Client &client, std::string const & key, int fromIndex,
            std::function<void(Response)> callback, bool recursive=false);
    Watcher(std::string const & etcd_url, std::string const & key,
            std::function<void(Response)> callback, bool recursive=false);
    Watcher(std::string const & etcd_url, std::string const & key, int fromIndex,
            std::function<void(Response)> callback, bool recursive=false);
    void Cancel();
    ~Watcher();

  protected:
    void doWatch(std::string const & key, std::function<void(Response)> callback);

    int index;
    std::function<void(Response)> callback;
    pplx::task<void> currentTask;
    std::unique_ptr<Watch::Stub> watchServiceStub;
    std::unique_ptr<KV::Stub> stub_;
    std::unique_ptr<etcdv3::AsyncWatchAction> call;

  private:
    int fromIndex;
    bool recursive;
  };
}

#endif
