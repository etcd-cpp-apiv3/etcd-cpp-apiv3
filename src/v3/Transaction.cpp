#include "etcd/v3/Transaction.hpp"

#include "proto/rpc.grpc.pb.h"

#include "etcd/v3/Action.hpp"

namespace etcdv3 {

namespace detail {

static etcdserverpb::Compare::CompareResult to_compare_result(CompareResult r) {
  return static_cast<etcdserverpb::Compare::CompareResult>(static_cast<int>(r));
}

static etcdserverpb::Compare::CompareTarget to_compare_target(CompareTarget t) {
  return static_cast<etcdserverpb::Compare::CompareTarget>(static_cast<int>(t));
}

}  // namespace detail

}  // namespace etcdv3

etcdv3::Transaction::Transaction() {
  txn_request.reset(new etcdserverpb::TxnRequest{});
}

etcdv3::Transaction::~Transaction() {}

void etcdv3::Transaction::add_compare(std::string const& key,
                                      CompareTarget const& target,
                                      CompareResult const& result,
                                      Value const& target_value,
                                      std::string const& range_end) {
  switch (target) {
  case CompareTarget::VERSION:
    add_compare_version(key, result, target_value.version, range_end);
    break;
  case CompareTarget::CREATE:
    add_compare_create(key, result, target_value.create_revision, range_end);
    break;
  case CompareTarget::MOD:
    add_compare_mod(key, result, target_value.mod_revision, range_end);
    break;
  case CompareTarget::VALUE:
    add_compare_value(key, result, target_value.value, range_end);
    break;
  case CompareTarget::LEASE:
    add_compare_lease(key, result, target_value.lease, range_end);
    break;
  default:
    // ignore invalid compare target
    break;
  }
}

void etcdv3::Transaction::add_compare_version(std::string const& key,
                                              int64_t const& version,
                                              std::string const& range_end) {
  this->add_compare_version(key, CompareResult::EQUAL, version, range_end);
}

void etcdv3::Transaction::add_compare_version(std::string const& key,
                                              CompareResult const& result,
                                              int64_t const& version,
                                              std::string const& range_end) {
  auto compare = txn_request->add_compare();
  compare->set_result(detail::to_compare_result(result));
  compare->set_target(detail::to_compare_target(CompareTarget::VERSION));
  compare->set_key(key);
  compare->set_version(version);
  compare->set_range_end(range_end);
}

void etcdv3::Transaction::add_compare_create(std::string const& key,
                                             int64_t const& create_revision,
                                             std::string const& range_end) {
  this->add_compare_create(key, CompareResult::EQUAL, create_revision,
                           range_end);
}

void etcdv3::Transaction::add_compare_create(std::string const& key,
                                             CompareResult const& result,
                                             int64_t const& create_revision,
                                             std::string const& range_end) {
  auto compare = txn_request->add_compare();
  compare->set_result(detail::to_compare_result(result));
  compare->set_target(detail::to_compare_target(CompareTarget::CREATE));
  compare->set_key(key);
  compare->set_create_revision(create_revision);
  compare->set_range_end(range_end);
}

void etcdv3::Transaction::add_compare_mod(std::string const& key,
                                          int64_t const& mod_revision,
                                          std::string const& range_end) {
  this->add_compare_mod(key, CompareResult::EQUAL, mod_revision, range_end);
}

void etcdv3::Transaction::add_compare_mod(std::string const& key,
                                          CompareResult const& result,
                                          int64_t const& mod_revision,
                                          std::string const& range_end) {
  auto compare = txn_request->add_compare();
  compare->set_result(detail::to_compare_result(result));
  compare->set_target(detail::to_compare_target(CompareTarget::MOD));
  compare->set_key(key);
  compare->set_mod_revision(mod_revision);
  compare->set_range_end(range_end);
}

void etcdv3::Transaction::add_compare_value(std::string const& key,
                                            std::string const& value,
                                            std::string const& range_end) {
  this->add_compare_value(key, CompareResult::EQUAL, value, range_end);
}

void etcdv3::Transaction::add_compare_value(std::string const& key,
                                            CompareResult const& result,
                                            std::string const& value,
                                            std::string const& range_end) {
  auto compare = txn_request->add_compare();
  compare->set_result(detail::to_compare_result(result));
  compare->set_target(detail::to_compare_target(CompareTarget::VALUE));
  compare->set_key(key);
  compare->set_value(value);
  compare->set_range_end(range_end);
}

void etcdv3::Transaction::add_compare_lease(std::string const& key,
                                            int64_t const& lease,
                                            std::string const& range_end) {
  this->add_compare_lease(key, CompareResult::EQUAL, lease, range_end);
}

void etcdv3::Transaction::add_compare_lease(std::string const& key,
                                            CompareResult const& result,
                                            int64_t const& lease,
                                            std::string const& range_end) {
  auto compare = txn_request->add_compare();
  compare->set_result(detail::to_compare_result(result));
  compare->set_target(detail::to_compare_target(CompareTarget::LEASE));
  compare->set_key(key);
  compare->set_lease(lease);
  compare->set_range_end(range_end);
}

void etcdv3::Transaction::add_success_range(std::string const& key,
                                            std::string const& range_end,
                                            bool const recursive,
                                            const int64_t limit) {
  auto succ = txn_request->add_success();
  auto get_request = succ->mutable_request_range();
  etcdv3::detail::make_request_with_ranges(*get_request, key, range_end,
                                           recursive);
  get_request->set_limit(limit);
}

void etcdv3::Transaction::add_success_put(std::string const& key,
                                          std::string const& value,
                                          int64_t const leaseid,
                                          const bool prev_kv) {
  auto succ = txn_request->add_success();
  auto put_request = succ->mutable_request_put();
  put_request->set_key(key);
  put_request->set_value(value);
  put_request->set_prev_kv(prev_kv);
  put_request->set_lease(leaseid);
}

void etcdv3::Transaction::add_success_delete(std::string const& key,
                                             std::string const& range_end,
                                             bool const recursive,
                                             const bool prev_kv) {
  auto succ = txn_request->add_success();
  auto del_request = succ->mutable_request_delete_range();
  etcdv3::detail::make_request_with_ranges(*del_request, key, range_end,
                                           recursive);
  del_request->set_prev_kv(prev_kv);
}

void etcdv3::Transaction::add_success_txn(
    const std::shared_ptr<Transaction> txn) {
  auto succ = txn_request->add_success();
  auto txn_request = succ->mutable_request_txn();
  txn_request->CopyFrom(*txn->txn_request);
}

void etcdv3::Transaction::add_failure_range(std::string const& key,
                                            std::string const& range_end,
                                            bool const recursive,
                                            const int64_t limit) {
  auto fail = txn_request->add_failure();
  auto get_request = fail->mutable_request_range();
  etcdv3::detail::make_request_with_ranges(*get_request, key, range_end,
                                           recursive);
  get_request->set_limit(limit);
}

void etcdv3::Transaction::add_failure_put(std::string const& key,
                                          std::string const& value,
                                          int64_t const leaseid,
                                          const bool prev_kv) {
  auto fail = txn_request->add_failure();
  auto put_request = fail->mutable_request_put();
  put_request->set_key(key);
  put_request->set_value(value);
  put_request->set_prev_kv(prev_kv);
  put_request->set_lease(leaseid);
}

void etcdv3::Transaction::add_failure_delete(std::string const& key,
                                             std::string const& range_end,
                                             bool const recursive,
                                             const bool prev_kv) {
  auto fail = txn_request->add_failure();
  auto del_request = fail->mutable_request_delete_range();
  etcdv3::detail::make_request_with_ranges(*del_request, key, range_end,
                                           recursive);
  del_request->set_prev_kv(prev_kv);
}

void etcdv3::Transaction::add_failure_txn(
    const std::shared_ptr<Transaction> txn) {
  auto fail = txn_request->add_failure();
  auto txn_request = fail->mutable_request_txn();
  txn_request->CopyFrom(*txn->txn_request);
}

void etcdv3::Transaction::setup_compare_and_create(
    std::string const& key, std::string const& prev_value,
    std::string const& create_key, std::string const& value,
    int64_t const leaseid) {
  this->add_compare_value(key, CompareResult::EQUAL, prev_value);
  this->add_success_put(create_key, value, leaseid);
  this->add_failure_range(key);
}

void etcdv3::Transaction::setup_compare_or_create(std::string const& key,
                                                  std::string const& prev_value,
                                                  std::string const& create_key,
                                                  std::string const& value,
                                                  int64_t const leaseid) {
  this->add_compare_value(key, CompareResult::NOT_EQUAL, prev_value);
  this->add_success_put(create_key, value, leaseid);
  this->add_failure_range(key);
}

void etcdv3::Transaction::setup_compare_and_swap(std::string const& key,
                                                 std::string const& prev_value,
                                                 std::string const& value,
                                                 int64_t const leaseid) {
  this->add_compare_value(key, CompareResult::EQUAL, prev_value);
  this->add_success_put(key, value, leaseid);
  this->add_failure_range(key);
}

void etcdv3::Transaction::setup_compare_or_swap(std::string const& key,
                                                std::string const& prev_value,
                                                std::string const& value,
                                                int64_t const leaseid) {
  this->add_compare_value(key, CompareResult::NOT_EQUAL, prev_value);
  this->add_success_put(key, value, leaseid);
  this->add_failure_range(key);
}

void etcdv3::Transaction::setup_compare_and_delete(
    std::string const& key, std::string const& prev_value,
    std::string const& delete_key, std::string const& range_end,
    const bool recursive) {
  this->add_compare_value(key, CompareResult::EQUAL, prev_value);
  this->add_success_delete(delete_key, range_end, recursive,
                           true /* for backwards compatibility */);
  this->add_failure_range(key);
}

void etcdv3::Transaction::setup_compare_or_delete(std::string const& key,
                                                  std::string const& prev_value,
                                                  std::string const& delete_key,
                                                  std::string const& range_end,
                                                  const bool recursive) {
  this->add_compare_value(key, CompareResult::NOT_EQUAL, prev_value);
  this->add_success_delete(delete_key, range_end, recursive,
                           true /* for backwards compatibility */);
  this->add_failure_range(key);
}

void etcdv3::Transaction::setup_compare_and_create(
    std::string const& key, const int64_t prev_revision,
    std::string const& create_key, std::string const& value,
    int64_t const leaseid) {
  this->add_compare_mod(key, CompareResult::EQUAL, prev_revision);
  this->add_success_put(create_key, value, leaseid);
  this->add_failure_range(key);
}

void etcdv3::Transaction::setup_compare_or_create(std::string const& key,
                                                  const int64_t prev_revision,
                                                  std::string const& create_key,
                                                  std::string const& value,
                                                  int64_t const leaseid) {
  this->add_compare_mod(key, CompareResult::NOT_EQUAL, prev_revision);
  this->add_success_put(create_key, value, leaseid);
  this->add_failure_range(key);
}

void etcdv3::Transaction::setup_compare_and_swap(std::string const& key,
                                                 const int64_t prev_revision,
                                                 std::string const& value,
                                                 int64_t const leaseid) {
  this->add_compare_mod(key, CompareResult::EQUAL, prev_revision);
  this->add_success_put(key, value, leaseid);
  this->add_failure_range(key);
}

void etcdv3::Transaction::setup_compare_or_swap(std::string const& key,
                                                const int64_t prev_revision,
                                                std::string const& value,
                                                int64_t const leaseid) {
  this->add_compare_mod(key, CompareResult::NOT_EQUAL, prev_revision);
  this->add_success_put(key, value, leaseid);
  this->add_failure_range(key);
}

void etcdv3::Transaction::setup_compare_and_delete(
    std::string const& key, const int64_t prev_revision,
    std::string const& delete_key, std::string const& range_end,
    const bool recursive) {
  this->add_compare_mod(key, CompareResult::EQUAL, prev_revision);
  this->add_success_delete(delete_key, range_end, recursive,
                           true /* for backwards compatibility */);
  this->add_failure_range(key);
}

void etcdv3::Transaction::setup_compare_or_delete(std::string const& key,
                                                  const int64_t prev_revision,
                                                  std::string const& delete_key,
                                                  std::string const& range_end,
                                                  const bool recursive) {
  this->add_compare_mod(key, CompareResult::NOT_EQUAL, prev_revision);
  this->add_success_delete(delete_key, range_end, recursive,
                           true /* for backwards compatibility */);
  this->add_failure_range(key);
}

void etcdv3::Transaction::setup_put(std::string const& key,
                                    std::string const& value) {
  this->add_success_put(key, value);
}

void etcdv3::Transaction::setup_delete(std::string const& key) {
  this->add_success_delete(key, "", false,
                           true /* for backwards compatibility */);
}

void etcdv3::Transaction::setup_delete(std::string const& key,
                                       std::string const& range_end,
                                       const bool recursive) {
  this->add_success_delete(key, range_end, recursive,
                           true /* for backwards compatibility */);
}
