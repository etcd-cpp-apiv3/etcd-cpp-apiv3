#if defined(_WIN32)
// see also:
// https://stackoverflow.com/questions/2561368/illegal-token-on-right-side-of
#define NOMINMAX
#endif

#include <pplx/pplxtasks.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "etcd/SyncClient.hpp"
#include "etcd/Value.hpp"

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#endif

#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>

#include <grpc++/grpc++.h>
#include <grpc++/security/credentials.h>
#include "proto/rpc.grpc.pb.h"
#include "proto/v3election.grpc.pb.h"
#include "proto/v3lock.grpc.pb.h"

#include "etcd/Client.hpp"
#include "etcd/KeepAlive.hpp"
#include "etcd/Watcher.hpp"
#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncGRPC.hpp"
#include "etcd/v3/Transaction.hpp"
#include "etcd/v3/action_constants.hpp"

etcd::Client::Client(etcd::SyncClient* client) : client(client) {
  this->own_client = false;
}

etcd::Client* WithClient(etcd::SyncClient* client) {
  return new etcd::Client(client);
}

etcd::Client::Client(std::string const& address,
                     std::string const& load_balancer) {
  this->own_client = true;
  this->client = new SyncClient(address, load_balancer);
}

etcd::Client::Client(std::string const& address,
                     grpc::ChannelArguments const& arguments) {
  this->own_client = true;
  this->client = new SyncClient(address, arguments);
}

etcd::Client* etcd::Client::WithUrl(std::string const& etcd_url,
                                    std::string const& load_balancer) {
  return new etcd::Client(etcd_url, load_balancer);
}

etcd::Client* etcd::Client::WithUrl(std::string const& etcd_url,
                                    grpc::ChannelArguments const& arguments) {
  return new etcd::Client(etcd_url, arguments);
}

etcd::Client::Client(std::string const& address, std::string const& username,
                     std::string const& password, int const auth_token_ttl,
                     std::string const& load_balancer) {
  this->own_client = true;
  this->client = new SyncClient(address, username, password, auth_token_ttl,
                                load_balancer);
}

etcd::Client::Client(std::string const& address, std::string const& username,
                     std::string const& password, int const auth_token_ttl,
                     grpc::ChannelArguments const& arguments) {
  this->own_client = true;
  this->client =
      new SyncClient(address, username, password, auth_token_ttl, arguments);
}

etcd::Client* etcd::Client::WithUser(std::string const& etcd_url,
                                     std::string const& username,
                                     std::string const& password,
                                     int const auth_token_ttl,
                                     std::string const& load_balancer) {
  return new etcd::Client(etcd_url, username, password, auth_token_ttl,
                          load_balancer);
}

etcd::Client* etcd::Client::WithUser(std::string const& etcd_url,
                                     std::string const& username,
                                     std::string const& password,
                                     int const auth_token_ttl,
                                     grpc::ChannelArguments const& arguments) {
  return new etcd::Client(etcd_url, username, password, auth_token_ttl,
                          arguments);
}

etcd::Client::Client(std::string const& address, std::string const& ca,
                     std::string const& cert, std::string const& privkey,
                     std::string const& target_name_override,
                     std::string const& load_balancer) {
  this->own_client = true;
  this->client = new SyncClient(address, ca, cert, privkey,
                                target_name_override, load_balancer);
}

etcd::Client::Client(std::string const& address, std::string const& ca,
                     std::string const& cert, std::string const& privkey,
                     std::string const& target_name_override,
                     grpc::ChannelArguments const& arguments) {
  this->own_client = true;
  this->client = new SyncClient(address, ca, cert, privkey,
                                target_name_override, arguments);
}

etcd::Client* etcd::Client::WithSSL(std::string const& etcd_url,
                                    std::string const& ca,
                                    std::string const& cert,
                                    std::string const& privkey,
                                    std::string const& target_name_override,
                                    std::string const& load_balancer) {
  return new etcd::Client(etcd_url, ca, cert, privkey, target_name_override,
                          load_balancer);
}

etcd::Client* etcd::Client::WithSSL(std::string const& etcd_url,
                                    grpc::ChannelArguments const& arguments,
                                    std::string const& ca,
                                    std::string const& cert,
                                    std::string const& privkey,
                                    std::string const& target_name_override) {
  return new etcd::Client(etcd_url, ca, cert, privkey, target_name_override,
                          arguments);
}

etcd::Client::~Client() {
  if (this->own_client) {
    delete this->client;
  }
}

namespace etcd {
namespace detail {

// Pass rvalue reference to lambda,
//
// Inspired by: https://stackoverflow.com/a/20669290/5080177

template <typename A, typename F>
class capture_impl {
  mutable A a;
  mutable F f;

 public:
  capture_impl(A&& a, F&& f) : a{std::forward<A>(a)}, f{std::forward<F>(f)} {}

  template <typename... Ts>
  auto operator()(Ts&&... args)
      -> decltype(std::forward<F>(f)(std::forward<A>(a),
                                     std::forward<Ts>(args)...)) {
    return std::forward<F>(f)(std::forward<A>(a), std::forward<Ts>(args)...);
  }

  template <typename... Ts>
  auto operator()(Ts&&... args) const
      -> decltype(std::forward<F>(f)(std::forward<A>(a),
                                     std::forward<Ts>(args)...)) {
    return std::forward<F>(f)(std::forward<A>(a), std::forward<Ts>(args)...);
  }
};

template <typename A, typename F>
static capture_impl<A, F> capture(A&& a, F&& f) {
  return capture_impl<A, F>(std::forward<A>(a), std::forward<F>(f));
}

template <typename F, typename Params, typename... Ts>
static auto asyncify(F&& fn, Params&& params, Ts... args)
    -> pplx::task<decltype(capture(std::forward<Params>(params),
                                   std::bind(std::forward<F>(fn),
                                             std::placeholders::_1,
                                             std::forward<Ts>(args)...))())> {
  return pplx::task<decltype(
      capture(std::forward<Params>(params),
              std::bind(std::forward<F>(fn), std::placeholders::_1,
                        std::forward<Ts>(args)...))())>(
      capture(std::forward<Params>(params),
              std::bind(std::forward<F>(fn), std::placeholders::_1,
                        std::forward<Ts>(args)...)));
}

}  // namespace detail

template <typename T>
using responser_t = etcd::Response (*)(std::shared_ptr<T>);

}  // namespace etcd

pplx::task<etcd::Response> etcd::Client::head() {
  return etcd::detail::asyncify(
      static_cast<etcd::responser_t<etcdv3::AsyncHeadAction>>(Response::create),
      this->client->head_internal());
}

pplx::task<etcd::Response> etcd::Client::get(std::string const& key) {
  return etcd::detail::asyncify(
      static_cast<etcd::responser_t<etcdv3::AsyncRangeAction>>(
          Response::create),
      this->client->get_internal(key));
}

pplx::task<etcd::Response> etcd::Client::get(std::string const& key,
                                             int64_t revision) {
  return etcd::detail::asyncify(
      static_cast<etcd::responser_t<etcdv3::AsyncRangeAction>>(
          Response::create),
      this->client->get_internal(key, revision));
}

pplx::task<etcd::Response> etcd::Client::set(std::string const& key,
                                             std::string const& value,
                                             const int64_t leaseid) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncPutAction>>(Response::create),
      this->client->put_internal(key, value, leaseid));
}

pplx::task<etcd::Response> etcd::Client::add(std::string const& key,
                                             std::string const& value,
                                             const int64_t leaseid) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncSetAction>>(Response::create),
      this->client->add_internal(key, value, leaseid));
}

pplx::task<etcd::Response> etcd::Client::put(std::string const& key,
                                             std::string const& value) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncPutAction>>(Response::create),
      this->client->put_internal(key, value));
}

pplx::task<etcd::Response> etcd::Client::put(std::string const& key,
                                             std::string const& value,
                                             const int64_t leaseId) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncPutAction>>(Response::create),
      this->client->put_internal(key, value, leaseId));
}

pplx::task<etcd::Response> etcd::Client::modify(std::string const& key,
                                                std::string const& value,
                                                const int64_t leaseid) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncUpdateAction>>(Response::create),
      this->client->modify_internal(key, value, leaseid));
}

pplx::task<etcd::Response> etcd::Client::modify_if(std::string const& key,
                                                   std::string const& value,
                                                   std::string const& old_value,
                                                   const int64_t leaseid) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncCompareAndSwapAction>>(
          Response::create),
      this->client->modify_if_internal(key, value, 0, old_value,
                                       etcdv3::AtomicityType::PREV_VALUE,
                                       leaseid));
}

pplx::task<etcd::Response> etcd::Client::modify_if(std::string const& key,
                                                   std::string const& value,
                                                   int64_t old_index,
                                                   const int64_t leaseid) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncCompareAndSwapAction>>(
          Response::create),
      this->client->modify_if_internal(key, value, old_index, "",
                                       etcdv3::AtomicityType::PREV_INDEX,
                                       leaseid));
}

pplx::task<etcd::Response> etcd::Client::rm(std::string const& key) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncDeleteAction>>(Response::create),
      this->client->rm_internal(key));
}

pplx::task<etcd::Response> etcd::Client::rm_if(std::string const& key,
                                               std::string const& old_value) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncCompareAndDeleteAction>>(
          Response::create),
      this->client->rm_if_internal(key, 0, old_value,
                                   etcdv3::AtomicityType::PREV_VALUE));
}

pplx::task<etcd::Response> etcd::Client::rm_if(std::string const& key,
                                               int64_t old_index) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncCompareAndDeleteAction>>(
          Response::create),
      this->client->rm_if_internal(key, old_index, "",
                                   etcdv3::AtomicityType::PREV_INDEX));
}

pplx::task<etcd::Response> etcd::Client::rmdir(std::string const& key,
                                               bool recursive) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncDeleteAction>>(Response::create),
      this->client->rmdir_internal(key, recursive));
}

pplx::task<etcd::Response> etcd::Client::rmdir(std::string const& key,
                                               const char* range_end) {
  return this->rmdir(key, std::string(range_end));
}

pplx::task<etcd::Response> etcd::Client::rmdir(std::string const& key,
                                               std::string const& range_end) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncDeleteAction>>(Response::create),
      this->client->rmdir_internal(key, range_end));
}

pplx::task<etcd::Response> etcd::Client::ls(std::string const& key) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncRangeAction>>(Response::create),
      this->client->ls_internal(key, 0));
}

pplx::task<etcd::Response> etcd::Client::ls(std::string const& key,
                                            size_t const limit) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncRangeAction>>(Response::create),
      this->client->ls_internal(key, limit));
}

pplx::task<etcd::Response> etcd::Client::ls(std::string const& key,
                                            size_t const limit,
                                            int64_t revision) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncRangeAction>>(Response::create),
      this->client->ls_internal(key, limit, false, revision));
}

pplx::task<etcd::Response> etcd::Client::ls(std::string const& key,
                                            std::string const& range_end) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncRangeAction>>(Response::create),
      this->client->ls_internal(key, range_end, 0));
}

pplx::task<etcd::Response> etcd::Client::ls(std::string const& key,
                                            std::string const& range_end,
                                            size_t const limit) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncRangeAction>>(Response::create),
      this->client->ls_internal(key, range_end, limit));
}

pplx::task<etcd::Response> etcd::Client::ls(std::string const& key,
                                            std::string const& range_end,
                                            size_t const limit,
                                            int64_t revision) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncRangeAction>>(Response::create),
      this->client->ls_internal(key, range_end, limit, false, revision));
}

pplx::task<etcd::Response> etcd::Client::keys(std::string const& key) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncRangeAction>>(Response::create),
      this->client->ls_internal(key, 0, true));
}

pplx::task<etcd::Response> etcd::Client::keys(std::string const& key,
                                              size_t const limit) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncRangeAction>>(Response::create),
      this->client->ls_internal(key, limit, true));
}

pplx::task<etcd::Response> etcd::Client::keys(std::string const& key,
                                              size_t const limit,
                                              int64_t revision) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncRangeAction>>(Response::create),
      this->client->ls_internal(key, limit, true, revision));
}

pplx::task<etcd::Response> etcd::Client::keys(std::string const& key,
                                              std::string const& range_end) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncRangeAction>>(Response::create),
      this->client->ls_internal(key, range_end, 0, true));
}

pplx::task<etcd::Response> etcd::Client::keys(std::string const& key,
                                              std::string const& range_end,
                                              size_t const limit) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncRangeAction>>(Response::create),
      this->client->ls_internal(key, range_end, limit, true));
}

pplx::task<etcd::Response> etcd::Client::keys(std::string const& key,
                                              std::string const& range_end,
                                              size_t const limit,
                                              int64_t revision) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncRangeAction>>(Response::create),
      this->client->ls_internal(key, range_end, limit, true, revision));
}

pplx::task<etcd::Response> etcd::Client::watch(std::string const& key,
                                               bool recursive) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncWatchAction>>(Response::create),
      this->client->watch_internal(key, 0, recursive));
}

pplx::task<etcd::Response> etcd::Client::watch(std::string const& key,
                                               int64_t fromIndex,
                                               bool recursive) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncWatchAction>>(Response::create),
      this->client->watch_internal(key, fromIndex, recursive));
}

pplx::task<etcd::Response> etcd::Client::watch(std::string const& key,
                                               const char* range_end) {
  return this->watch(key, std::string(range_end));
}

pplx::task<etcd::Response> etcd::Client::watch(std::string const& key,
                                               std::string const& range_end) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncWatchAction>>(Response::create),
      this->client->watch_internal(key, range_end, 0));
}

pplx::task<etcd::Response> etcd::Client::watch(std::string const& key,
                                               std::string const& range_end,
                                               int64_t fromIndex) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncWatchAction>>(Response::create),
      this->client->watch_internal(key, range_end, fromIndex));
}

pplx::task<etcd::Response> etcd::Client::leasegrant(int ttl) {
  // See Note [lease with TTL and issue the actual request]
  return pplx::task<etcd::Response>(
      [this, ttl]() { return this->client->leasegrant(ttl); });
}

pplx::task<std::shared_ptr<etcd::KeepAlive>> etcd::Client::leasekeepalive(
    int ttl) {
  // See Note [lease with TTL and issue the actual request]
  return pplx::task<std::shared_ptr<etcd::KeepAlive>>(
      [this, ttl]() { return this->client->leasekeepalive(ttl); });
}

pplx::task<etcd::Response> etcd::Client::leaserevoke(int64_t lease_id) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncLeaseRevokeAction>>(
          Response::create),
      this->client->leaserevoke_internal(lease_id));
}

pplx::task<etcd::Response> etcd::Client::leasetimetolive(int64_t lease_id) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncLeaseTimeToLiveAction>>(
          Response::create),
      this->client->leasetimetolive_internal(lease_id));
}

pplx::task<etcd::Response> etcd::Client::leases() {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncLeaseLeasesAction>>(
          Response::create),
      this->client->leases_internal());
}

pplx::task<etcd::Response> etcd::Client::add_member(
    std::string const& peer_urls, bool is_learner) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncAddMemberAction>>(Response::create),
      this->client->add_member_internal(peer_urls, is_learner));
}

pplx::task<etcd::Response> etcd::Client::list_member() {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncListMemberAction>>(Response::create),
      this->client->list_member_internal());
}

pplx::task<etcd::Response> etcd::Client::remove_member(
    const uint64_t member_id) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncRemoveMemberAction>>(
          Response::create),
      this->client->remove_member_internal(member_id));
}

pplx::task<etcd::Response> etcd::Client::lock(std::string const& key) {
  static const int DEFAULT_LEASE_TTL_FOR_LOCK =
      10;  // see also etcd::SyncClient::lock
  return this->lock(key, DEFAULT_LEASE_TTL_FOR_LOCK);
}

pplx::task<etcd::Response> etcd::Client::lock(std::string const& key,
                                              const bool sync) {
  static const int DEFAULT_LEASE_TTL_FOR_LOCK =
      10;  // see also etcd::SyncClient::lock
  return this->lock(key, DEFAULT_LEASE_TTL_FOR_LOCK, sync);
}

pplx::task<etcd::Response> etcd::Client::lock(std::string const& key,
                                              int lease_ttl) {
  // See Note [lease with TTL and issue the actual request]
  // See also SyncClient::lock
  //
  // We don't separate the keepalive step and lock step to avoid leaving many
  // busy-waiting keepalive tasks when waiting for the locks due to the
  // contention of async threadpool.
  return pplx::task<etcd::Response>(
      [this, key, lease_ttl]() { return this->client->lock(key, lease_ttl); });
}

pplx::task<etcd::Response> etcd::Client::lock(std::string const& key,
                                              int lease_ttl, const bool sync) {
  if (sync) {
    pplx::task_completion_event<etcd::Response> event;
    event.set(this->client->lock(key, lease_ttl));
    return pplx::task<etcd::Response>(event);
  } else {
    return this->lock(key, lease_ttl);
  }
}

pplx::task<etcd::Response> etcd::Client::lock_with_lease(std::string const& key,
                                                         int64_t lease_id) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncLockAction>>(Response::create),
      this->client->lock_with_lease_internal(key, lease_id));
}

pplx::task<etcd::Response> etcd::Client::lock_with_lease(std::string const& key,
                                                         int64_t lease_id,
                                                         const bool sync) {
  if (sync) {
    pplx::task_completion_event<etcd::Response> event;
    event.set(this->client->lock_with_lease(key, lease_id));
    return pplx::task<etcd::Response>(event);
  } else {
    return this->lock_with_lease(key, lease_id);
  }
}

pplx::task<etcd::Response> etcd::Client::unlock(std::string const& lock_key) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncUnlockAction>>(Response::create),
      this->client->unlock_internal(lock_key));
}

pplx::task<etcd::Response> etcd::Client::txn(etcdv3::Transaction const& txn) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncTxnAction>>(Response::create),
      this->client->txn_internal(txn));
}

pplx::task<etcd::Response> etcd::Client::campaign(std::string const& name,
                                                  int64_t lease_id,
                                                  std::string const& value) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncCampaignAction>>(Response::create),
      this->client->campaign_internal(name, lease_id, value));
}

pplx::task<etcd::Response> etcd::Client::proclaim(std::string const& name,
                                                  int64_t lease_id,
                                                  std::string const& key,
                                                  int64_t revision,
                                                  std::string const& value) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncProclaimAction>>(Response::create),
      this->client->proclaim_internal(name, lease_id, key, revision, value));
}

pplx::task<etcd::Response> etcd::Client::leader(std::string const& name) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncLeaderAction>>(Response::create),
      this->client->leader_internal(name));
}

std::unique_ptr<etcd::Client::Observer> etcd::Client::observe(
    std::string const& name) {
  return this->client->observe(name);
}

pplx::task<etcd::Response> etcd::Client::resign(std::string const& name,
                                                int64_t lease_id,
                                                std::string const& key,
                                                int64_t revision) {
  return etcd::detail::asyncify(
      static_cast<responser_t<etcdv3::AsyncResignAction>>(Response::create),
      this->client->resign_internal(name, lease_id, key, revision));
}

const std::string& etcd::Client::current_auth_token() const {
  return this->client->current_auth_token();
}

std::shared_ptr<grpc::Channel> etcd::Client::grpc_channel() const {
  return this->client->grpc_channel();
}

etcd::SyncClient* etcd::Client::sync_client() const { return this->client; }

// watchers from the async client

etcd::Watcher::Watcher(Client const& client, std::string const& key,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback, bool recursive)
    : Watcher(*client.sync_client(), key, callback, wait_callback, recursive) {}

etcd::Watcher::Watcher(Client const& client, std::string const& key,
                       std::string const& range_end,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback)
    : Watcher(*client.sync_client(), key, range_end, callback, wait_callback) {}

etcd::Watcher::Watcher(Client const& client, std::string const& key,
                       int64_t fromIndex,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback, bool recursive)
    : Watcher(*client.sync_client(), key, fromIndex, callback, wait_callback,
              recursive) {}

etcd::Watcher::Watcher(Client const& client, std::string const& key,
                       std::string const& range_end, int64_t fromIndex,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback)
    : Watcher(*client.sync_client(), key, range_end, fromIndex, callback,
              wait_callback) {}

etcd::Watcher::Watcher(Client const& client, std::string const& key,
                       std::function<void(Response)> callback, bool recursive)
    : Watcher(*client.sync_client(), key, callback, recursive) {}

etcd::Watcher::Watcher(Client const& client, std::string const& key,
                       std::string const& range_end,
                       std::function<void(Response)> callback)
    : Watcher(*client.sync_client(), key, range_end, callback) {}

etcd::Watcher::Watcher(Client const& client, std::string const& key,
                       int64_t fromIndex,
                       std::function<void(Response)> callback, bool recursive)
    : Watcher(*client.sync_client(), key, fromIndex, callback, recursive) {}

etcd::Watcher::Watcher(Client const& client, std::string const& key,
                       std::string const& range_end, int64_t fromIndex,
                       std::function<void(Response)> callback)
    : Watcher(*client.sync_client(), key, range_end, fromIndex, callback) {}

// keepalive from then async client

etcd::KeepAlive::KeepAlive(Client const& client, int ttl, int64_t lease_id)
    : KeepAlive(*client.sync_client(), ttl, lease_id) {}

etcd::KeepAlive::KeepAlive(
    Client const& client,
    std::function<void(std::exception_ptr)> const& handler, int ttl,
    int64_t lease_id)
    : KeepAlive(*client.sync_client(), handler, ttl, lease_id) {}
