#ifndef __ETCD_RESPONSE_HPP__
#define __ETCD_RESPONSE_HPP__

#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "etcd/Value.hpp"

namespace etcdv3 {
class AsyncWatchAction;
class AsyncLeaseKeepAliveAction;
class AsyncObserveAction;
class V3Response;
}  // namespace etcdv3

namespace etcd {
typedef std::vector<std::string> Keys;

namespace detail {
// Compute the duration between given start time point and now
inline const std::chrono::microseconds duration_till_now(
    std::chrono::high_resolution_clock::time_point const& start_timepoint) {
  return std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::high_resolution_clock::now() - start_timepoint);
}
}  // namespace detail

// forward declaration
class KeepAlive;
class Watcher;

/**
 * The Response object received for the requests of etcd::Client
 */
class Response {
 public:
  template <typename T>
  static etcd::Response create(std::unique_ptr<T> call) {
    call->waitForResponse();
    auto v3resp = call->ParseResponse();
    return etcd::Response(v3resp,
                          detail::duration_till_now(call->startTimepoint()));
  }

  template <typename T>
  static etcd::Response create(std::shared_ptr<T> call) {
    call->waitForResponse();
    auto v3resp = call->ParseResponse();
    return etcd::Response(v3resp,
                          detail::duration_till_now(call->startTimepoint()));
  }

  template <typename T>
  static etcd::Response create(std::unique_ptr<T> call,
                               std::function<void(Response)> callback) {
    call->waitForResponse(callback);
    auto v3resp = call->ParseResponse();
    return etcd::Response(v3resp,
                          detail::duration_till_now(call->startTimepoint()));
  }

  template <typename T>
  static etcd::Response create(std::function<std::unique_ptr<T>()> fn) {
    auto call = fn();

    call->waitForResponse();
    auto v3resp = call->ParseResponse();
    return etcd::Response(v3resp,
                          detail::duration_till_now(call->startTimepoint()));
  }

  template <typename T>
  static etcd::Response create(std::function<std::shared_ptr<T>()> fn) {
    auto call = fn();

    call->waitForResponse();
    auto v3resp = call->ParseResponse();
    return etcd::Response(v3resp,
                          detail::duration_till_now(call->startTimepoint()));
  }

  Response();

  Response(const Response&);

  /**
   * Returns the error code received from the etcd server. In case of success
   * the error code is 0.
   */
  int error_code() const;

  /**
   * Returns the string representation of the error code
   */
  std::string const& error_message() const;

  /**
   * Returns true if this is a successful response
   */
  bool is_ok() const;

  /**
   * Returns true if the error is a network unavailable error.
   */
  bool is_network_unavailable() const;

  /**
   * Check whether the response contains a grpc TIMEOUT error.
   */
  bool is_grpc_timeout() const;

  /**
   * Returns the action type of the operation that this response belongs to.
   */
  std::string const& action() const;

  /**
   * Returns the current index value of etcd
   */
  int64_t index() const;

  /**
   * Returns the value object of the response to a get/set/modify operation.
   */
  Value const& value() const;

  /**
   * Returns the previous value object of the response to a set/modify/rm
   * operation.
   */
  Value const& prev_value() const;

  /**
   * Returns the index-th value of the response to an 'ls' operation. Equivalent
   * to values()[index]
   */
  Value const& value(int index) const;

  /**
   * Returns the vector of values in a directory in response to an 'ls'
   * operation.
   */
  Values const& values() const;

  /**
   * Returns the vector of keys in a directory in response to an 'ls' operation.
   */
  Keys const& keys() const;

  /**
   * Returns the index-th key in a directory listing. Same as keys()[index]
   */
  std::string const& key(int index) const;

  /**
   * Returns the compact_revision if the response is a watch-cancelled revision.
   * `-1` means uninitialized (the response is not watch-cancelled)
   */
  int64_t compact_revision() const;

  /**
   * Returns the watcher id for client.watch() requests. `-1` means
   * uninitialized (the response is not for watch).
   */
  int64_t watch_id() const;

  /**
   * Returns the lock key.
   */
  std::string const& lock_key() const;

  /**
   * Return the "name" in response.
   */
  std::string const& name() const;

  /**
   * Returns the watched events.
   */
  std::vector<Event> const& events() const;

  /**
   * Returns the duration of request execution in microseconds.
   */
  std::chrono::microseconds const& duration() const;

  /**
   * Returns the current cluster id.
   */
  uint64_t cluster_id() const;

  /**
   * Returns the current member id.
   */
  uint64_t member_id() const;

  /**
   * Returns ther current raft term.
   */
  uint64_t raft_term() const;

  /**
   * Returns ther current raft term.
   */
  std::vector<int64_t> const& leases() const;

 protected:
  Response(const etcdv3::V3Response& response,
           std::chrono::microseconds const& duration);
  Response(int error_code, std::string const& error_message);
  Response(int error_code, char const* error_message);

  int _error_code;
  std::string _error_message;
  int64_t _index;
  std::string _action;
  Value _value;
  Value _prev_value;
  Values _values;
  Keys _keys;
  int64_t _compact_revision = -1;  // for watch
  int64_t _watch_id = -1;          // for watch
  std::string _lock_key;           // for lock
  std::string _name;               // for campaign (in v3election)
  std::vector<Event> _events;      // for watch
  // execute duration (in microseconds), during the action created and response
  // parsed
  std::chrono::microseconds _duration;

  // cluster metadata
  uint64_t _cluster_id;
  uint64_t _member_id;
  uint64_t _raft_term;

  // for lease list
  std::vector<int64_t> _leases;

  friend class Client;
  friend class SyncClient;
  friend class KeepAlive;
  friend class Watcher;

  friend class etcdv3::AsyncWatchAction;
  friend class etcdv3::AsyncLeaseKeepAliveAction;
  friend class etcdv3::AsyncObserveAction;
};
}  // namespace etcd

#endif
