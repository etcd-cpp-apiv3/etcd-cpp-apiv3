#ifndef __V3_ACTION_HPP__
#define __V3_ACTION_HPP__

#include <chrono>
#include <ostream>

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "proto/v3election.grpc.pb.h"
#include "proto/v3lock.grpc.pb.h"

#include "etcd/v3/action_constants.hpp"

using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

using etcdserverpb::Cluster;
using etcdserverpb::KV;
using etcdserverpb::Lease;
using etcdserverpb::Watch;
using v3electionpb::Election;
using v3lockpb::Lock;

namespace etcd {
class Response;
}

namespace etcdv3 {
enum class AtomicityType { PREV_INDEX = 0, PREV_VALUE = 1 };

struct ActionParameters {
  ActionParameters();
  bool withPrefix;
  int64_t revision = 0;
  int64_t old_revision = 0;
  int64_t lease_id = 0;  // no lease
  int ttl;
  int limit;
  std::string name;  // for campaign (in v3election)
  std::string key;
  std::string range_end;
  bool keys_only;
  bool count_only;
  std::string value;
  std::string old_value;
  std::string auth_token;

  // for cluster management apis
  std::vector<std::string> peer_urls;
  bool is_learner;
  uint64_t member_id;

  std::chrono::microseconds grpc_timeout = std::chrono::microseconds::zero();
  KV::Stub* kv_stub;
  Watch::Stub* watch_stub;
  Cluster::Stub* cluster_stub;
  Lease::Stub* lease_stub;
  Lock::Stub* lock_stub;
  Election::Stub* election_stub;

  bool has_grpc_timeout() const;
  std::chrono::system_clock::time_point grpc_deadline() const;

  void dump(std::ostream& os) const;
};

class Action {
 public:
  Action(etcdv3::ActionParameters const& params);
  Action(etcdv3::ActionParameters&& params);
  virtual ~Action();

  void waitForResponse();
  const std::chrono::high_resolution_clock::time_point startTimepoint();

 protected:
  Status status;
  ClientContext context;
  CompletionQueue cq_;
  etcdv3::ActionParameters parameters;
  std::chrono::high_resolution_clock::time_point start_timepoint;

 private:
  // Init things like auth token, etc.
  void InitAction();

  friend class etcd::Response;
};

namespace detail {
std::string string_plus_one(std::string const& value);
std::string resolve_etcd_endpoints(std::string const& default_endpoints);

template <typename Req>
void make_request_with_ranges(Req& req, std::string const& key,
                              std::string const& range_end,
                              bool const recursive) {
  if (!recursive) {
    req.set_key(key);
  } else {
    if (key.empty()) {
      req.set_key(etcdv3::NUL);
      req.set_range_end(etcdv3::NUL);
    } else {
      req.set_key(key);
      req.set_range_end(detail::string_plus_one(key));
    }
  }
  if (!range_end.empty()) {
    req.set_range_end(range_end);
  }
}

}  // namespace detail
}  // namespace etcdv3
#endif
