#ifndef __ETCD_KEEPALIVE_HPP__
#define __ETCD_KEEPALIVE_HPP__

#include <string>

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
   * If ID is set to 0, etcd will choose an ID.
   */
  class KeepAlive
  {
  public:
    KeepAlive(Client const &client, int ttl, int64_t lease_id=0);
    KeepAlive(std::string const & address, int ttl, int64_t lease_id=0);
    KeepAlive(std::string const & address,
              std::string const & username, std::string const & password,
              int ttl, int64_t lease_id=0);

    KeepAlive(KeepAlive const &) = delete;
    KeepAlive(KeepAlive &&) = delete;

    /**
     * Stop the keep alive action.
     */
    void Cancel();

    ~KeepAlive();

  protected:
    void refresh();

    pplx::task<void> currentTask;
    struct EtcdServerStubs;
    struct EtcdServerStubsDeleter {
      void operator()(EtcdServerStubs *stubs);
    };
    std::unique_ptr<EtcdServerStubs, EtcdServerStubsDeleter> stubs;

  private:
    int ttl;
    int64_t lease_id;
    bool continue_next;
#if BOOST_VERSION >= 106600
    boost::asio::io_context context;
#else
    boost::asio::io_service context;
#endif
    std::unique_ptr<boost::asio::steady_timer> keepalive_timer_;
  };
}

#endif
