#ifndef __ETCD_WATCHER_HPP__
#define __ETCD_WATCHER_HPP__

#include <atomic>
#include <functional>
#include <string>
#include <thread>

#include "etcd/Client.hpp"
#include "etcd/Response.hpp"

namespace etcd
{
  class Watcher
  {
  public:
    Watcher(Client const &client, std::string const & key,
            std::function<void(Response)> callback, bool recursive=false);
    Watcher(Client const &client, std::string const & key,
            std::string const &range_end,
            std::function<void(Response)> callback);
    Watcher(Client const &client, std::string const & key, int fromIndex,
            std::function<void(Response)> callback, bool recursive=false);
    Watcher(Client const &client, std::string const & key,
            std::string const &range_end, int fromIndex,
            std::function<void(Response)> callback);
    Watcher(std::string const & address, std::string const & key,
            std::function<void(Response)> callback, bool recursive=false);
    Watcher(std::string const & address, std::string const & key,
            std::string const &range_end,
            std::function<void(Response)> callback);
    Watcher(std::string const & address, std::string const & key, int fromIndex,
            std::function<void(Response)> callback, bool recursive=false);
    Watcher(std::string const & address, std::string const & key,
            std::string const &range_end, int fromIndex,
            std::function<void(Response)> callback);
    Watcher(std::string const & address,
            std::string const & username, std::string const & password,
            std::string const & key,
            std::function<void(Response)> callback, bool recursive=false,
            int const auth_token_ttl = 300);
    Watcher(std::string const & address,
            std::string const & username, std::string const & password,
            std::string const & key, std::string const &range_end,
            std::function<void(Response)> callback,
            int const auth_token_ttl = 300);
    Watcher(std::string const & address,
            std::string const & username, std::string const & password,
            std::string const & key, int fromIndex,
            std::function<void(Response)> callback, bool recursive=false,
            int const auth_token_ttl = 300);
    Watcher(std::string const & address,
            std::string const & username, std::string const & password,
            std::string const & key, std::string const &range_end, int fromIndex,
            std::function<void(Response)> callback,
            int const auth_token_ttl = 300);

    Watcher(Watcher const &) = delete;
    Watcher(Watcher &&) = delete;

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
                 std::string const & range_end,
                 std::string const & auth_token,
                 std::function<void(Response)> callback);

    int index;
    std::function<void(Response)> callback;
    std::function<void(bool)> wait_callback;

    // Don't use `pplx::task` to avoid sharing thread pool with other actions on the client
    // to avoid any potential blocking, which may block the keepalive loop and evict the lease.
    std::thread task_;

    struct EtcdServerStubs;
    struct EtcdServerStubsDeleter {
      void operator()(etcd::Watcher::EtcdServerStubs *stubs);
    };
    std::unique_ptr<EtcdServerStubs, EtcdServerStubsDeleter> stubs;

  private:
    int fromIndex;
    bool recursive;
    std::atomic_bool cancelled;
  };
}

#endif
