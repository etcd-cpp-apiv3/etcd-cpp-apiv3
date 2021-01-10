#ifndef __ETCD_KEEPALIVE_HPP__
#define __ETCD_KEEPALIVE_HPP__

#include <string>

#include "etcd/Client.hpp"
#include "etcd/Response.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"

namespace etcdv3 {
  class AsyncLeaseKeepAliveAction;
}

using etcdserverpb::KV;
using etcdserverpb::Lease;
using grpc::Channel;

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
    std::unique_ptr<Lease::Stub> leaseServiceStub;
    std::unique_ptr<etcdv3::AsyncLeaseKeepAliveAction> call;

  private:
    int ttl;
    int64_t lease_id;
    bool continue_next;
    boost::asio::io_context context;
    std::unique_ptr<boost::asio::steady_timer> keepalive_timer_;
  };
}

#endif
