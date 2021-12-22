#ifndef __ETCD_KEEPALIVE_HPP__
#define __ETCD_KEEPALIVE_HPP__

#include <atomic>
#include <exception>
#include <functional>
#include <string>
#include <thread>

#include "etcd/Client.hpp"
#include "etcd/Response.hpp"

#include <boost/config.hpp>
#if BOOST_VERSION >= 106600
#include <boost/asio/io_context.hpp>
#else
#include <boost/asio/io_service.hpp>
#endif
#include <boost/asio/steady_timer.hpp>

namespace etcd
{
  /**
   * If ID is set to 0, the library will choose an ID, and can be accessed from ".Lease()".
   */
  class KeepAlive
  {
  public:
    KeepAlive(Client const &client,
              int ttl, int64_t lease_id = 0);
    KeepAlive(std::string const & address,
              int ttl, int64_t lease_id = 0);
    KeepAlive(std::string const & address,
              std::string const & username, std::string const & password,
              int ttl, int64_t lease_id = 0,
              int const auth_token_ttl = 300);

    KeepAlive(Client const &client,
              std::function<void (std::exception_ptr)> const &handler,
              int ttl, int64_t lease_id = 0);
    KeepAlive(std::string const & address,
              std::function<void (std::exception_ptr)> const &handler,
              int ttl, int64_t lease_id = 0);
    KeepAlive(std::string const & address,
              std::string const & username, std::string const & password,
              std::function<void (std::exception_ptr)> const &handler,
              int ttl, int64_t lease_id = 0,
              int const auth_token_ttl = 300);

    KeepAlive(KeepAlive const &) = delete;
    KeepAlive(KeepAlive &&) = delete;

    int64_t Lease() const { return lease_id; }

    /**
     * Stop the keep alive action.
     */
    void Cancel();

    /**
     * Check if the keep alive is still valid (invalid when there's an async exception).
     *
     * Nothing will happen if valid and an exception will be rethrowed if invalid.
     */
    void Check();

    ~KeepAlive();

  protected:
    void refresh();

    struct EtcdServerStubs;
    struct EtcdServerStubsDeleter {
      void operator()(EtcdServerStubs *stubs);
    };
    std::unique_ptr<EtcdServerStubs, EtcdServerStubsDeleter> stubs;

  private:
    // error handling
    std::exception_ptr eptr_;
    std::function<void (std::exception_ptr)> handler_;

    // Don't use `pplx::task` to avoid sharing thread pool with other actions on the client
    // to avoid any potential blocking, which may block the keepalive loop and evict the lease.
    std::thread task_;

    int ttl;
    int64_t lease_id;
    std::atomic_bool continue_next;
#if BOOST_VERSION >= 106600
    boost::asio::io_context context;
#else
    boost::asio::io_service context;
#endif
    std::unique_ptr<boost::asio::steady_timer> keepalive_timer_;
  };
}

#endif
