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
    Watcher(Client const &client, std::string const & key,
            std::function<void(Response)> callback, bool recursive=false);
    Watcher(Client const &client, std::string const & key, int fromIndex,
            std::function<void(Response)> callback, bool recursive=false);
    Watcher(std::string const & address, std::string const & key,
            std::function<void(Response)> callback, bool recursive=false);
    Watcher(std::string const & address, std::string const & key, int fromIndex,
            std::function<void(Response)> callback, bool recursive=false);
    Watcher(std::string const & address,
            std::string const & username, std::string const & password,
            std::string const & key,
            std::function<void(Response)> callback, bool recursive=false);
    Watcher(std::string const & address,
            std::string const & username, std::string const & password,
            std::string const & key, int fromIndex,
            std::function<void(Response)> callback, bool recursive=false);

    /**
     * Wait util the task has been stopped, actively or passively, e.g., the watcher
     * get cancelled or the server closes the connection.
     *
     * Returns true if the watcher is been normally cancalled, otherwise false.
     */
    bool Wait();

    /**
     * An async wait, the callback will be called when the task has been stopped.
     *
     * The callback parameter would be true if the watch is been normally cancalled.
     */
    void Wait(std::function<void(bool)> callback);

    /**
     * Stop the watching action.
     */
    void Cancel();

    ~Watcher();

  protected:
    void doWatch(std::string const & key,
                 std::string const & auth_token,
                 std::function<void(Response)> callback);

    int index;
    std::function<void(Response)> callback;
    pplx::task<void> currentTask;
    std::unique_ptr<Watch::Stub> watchServiceStub;
    std::unique_ptr<etcdv3::AsyncWatchAction> call;

  private:
    int fromIndex;
    bool recursive;
  };
}

#endif
