#include "etcd/v3/V3Response.hpp"
#include "etcd/v3/Member.hpp"
#include "etcd/v3/action_constants.hpp"

void etcdv3::V3Response::set_error_code(int code) { error_code = code; }

void etcdv3::V3Response::set_error_message(std::string msg) {
  error_message = msg;
}

int64_t etcdv3::V3Response::get_index() const { return index; }

std::string const& etcdv3::V3Response::get_action() const { return action; }

int etcdv3::V3Response::get_error_code() const { return error_code; }

std::string const& etcdv3::V3Response::get_error_message() const {
  return error_message;
}

void etcdv3::V3Response::set_action(std::string action) {
  this->action = action;
}

std::vector<etcdv3::KeyValue> const& etcdv3::V3Response::get_values() const {
  return values;
}

std::vector<etcdv3::KeyValue> const& etcdv3::V3Response::get_prev_values()
    const {
  return prev_values;
}

etcdv3::KeyValue const& etcdv3::V3Response::get_value() const { return value; }

etcdv3::KeyValue const& etcdv3::V3Response::get_prev_value() const {
  return prev_value;
}

bool etcdv3::V3Response::has_values() const { return values.size() > 0; }

int64_t etcdv3::V3Response::get_compact_revision() const {
  return compact_revision;
}

void etcdv3::V3Response::set_compact_revision(const int64_t compact_revision) {
  this->compact_revision = compact_revision;
}

int64_t etcdv3::V3Response::get_watch_id() const { return watch_id; }

void etcdv3::V3Response::set_watch_id(const int64_t watch_id) {
  this->watch_id = watch_id;
}

void etcdv3::V3Response::set_lock_key(std::string const& key) {
  this->lock_key = key;
}

std::string const& etcdv3::V3Response::get_lock_key() const {
  return this->lock_key;
}

void etcdv3::V3Response::set_name(std::string const& name) {
  this->name = name;
}

std::string const& etcdv3::V3Response::get_name() const { return this->name; }

std::vector<mvccpb::Event> const& etcdv3::V3Response::get_events() const {
  return this->events;
}

uint64_t etcdv3::V3Response::get_cluster_id() const { return this->cluster_id; }

uint64_t etcdv3::V3Response::get_member_id() const { return this->member_id; }

uint64_t etcdv3::V3Response::get_raft_term() const { return this->raft_term; }

std::vector<int64_t> const& etcdv3::V3Response::get_leases() const {
  return this->leases;
}

std::vector<etcdv3::Member> const& etcdv3::V3Response::get_members() const {
  return this->members;
}
