#ifndef __ETCD_KEEPALIVE_HPP__
#define __ETCD_KEEPALIVE_HPP__

#include <atomic>
#include <chrono>
#include <exception>
#include <functional>
#include <string>
#include <thread>

#include "etcd/SyncClient.hpp"
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
  // forward declaration to avoid header/library dependency
  class Client;

  /**
   * If ID is set to 0, the library will choose an ID, and can be accessed from ".Lease()".
   */
  class KeepAlive
  {
  public:
    KeepAlive(Client const &client,
              int ttl, int64_t lease_id = 0);
    KeepAlive(SyncClient const &client,
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
    KeepAlive(SyncClient const &client,
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
    KeepAlive(std::string const & address,
              std::string const & ca,
              std::string const & cert,
              std::string const & privkey,
              std::function<void (std::exception_ptr)> const &handler,
              int ttl, int64_t lease_id = 0,
              std::string const & target_name_override = "");

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

    /**
     * Set a timeout value for grpc operations.
     */
    template <typename Rep = std::micro>
    void set_grpc_timeout(std::chrono::duration<Rep> const &timeout) {
      this->grpc_timeout = std::chrono::duration_cast<std::chrono::milliseconds>(timeout);
    }

    /**
     * Get the current timeout value for grpc operations.
     */
    std::chrono::microseconds get_grpc_timeout() const {
      return this->grpc_timeout;
    }

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

    // grpc timeout in `refresh()`
    mutable std::chrono::microseconds grpc_timeout = std::chrono::microseconds::zero();

#if BOOST_VERSION >= 106600
    boost::asio::io_context context;
#else
    boost::asio::io_service context;
#endif
    std::unique_ptr<boost::asio::steady_timer> keepalive_timer_;
  };
}

#endif
