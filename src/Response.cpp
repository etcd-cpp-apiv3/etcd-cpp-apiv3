#include "etcd/Response.hpp"
#include "etcd/v3/V3Response.hpp"

#include <iostream>

etcd::Response::Response() : _error_code(0), _index(0) {}

etcd::Response::Response(const etcd::Response& response) {
  this->_error_code = response._error_code;
  this->_error_message = response._error_message;
  this->_index = response._index;
  this->_action = response._action;
  this->_value = response._value;
  this->_prev_value = response._prev_value;
  this->_values = response._values;
  this->_keys = response._keys;
  this->_compact_revision = response._compact_revision;
  this->_lock_key = response._lock_key;
  this->_name = response._name;
  this->_events = response._events;
  this->_duration = response._duration;

  this->_cluster_id = response._cluster_id;
  this->_member_id = response._member_id;
  this->_raft_term = response._raft_term;

  this->_leases = response._leases;
  this->_members = response._members;
}

etcd::Response::Response(const etcdv3::V3Response& reply,
                         std::chrono::microseconds const& duration) {
  _index = reply.get_index();
  _action = reply.get_action();
  _error_code = reply.get_error_code();
  _error_message = reply.get_error_message();
  if (reply.has_values()) {
    auto val = reply.get_values();
    for (unsigned int index = 0; index < val.size(); index++) {
      _values.push_back(Value(val[index]));
      _keys.push_back(val[index].kvs.key());
    }
    _value = Value(reply.get_values()[0]);
  } else {
    _value = Value(reply.get_value());
  }
  _prev_value = Value(reply.get_prev_value());

  _compact_revision = reply.get_compact_revision();
  _watch_id = reply.get_watch_id();
  _lock_key = reply.get_lock_key();
  _name = reply.get_name();

  for (auto const& ev : reply.get_events()) {
    _events.emplace_back(etcd::Event(ev));
  }

  // duration
  _duration = duration;

  // etcd head
  _cluster_id = reply.get_cluster_id();
  _member_id = reply.get_member_id();
  _raft_term = reply.get_raft_term();

  // lease list
  this->_leases = reply.get_leases();
  // member list
  this->_members = reply.get_members();
}

etcd::Response::Response(int error_code, std::string const& error_message)
    : _error_code(error_code), _error_message(error_message), _index(0) {}

etcd::Response::Response(int error_code, char const* error_message)
    : _error_code(error_code), _error_message(error_message), _index(0) {}

int etcd::Response::error_code() const { return _error_code; }

std::string const& etcd::Response::error_message() const {
  return _error_message;
}

bool etcd::Response::is_ok() const { return error_code() == 0; }

bool etcd::Response::is_network_unavailable() const {
  return error_code() == ::grpc::StatusCode::UNAVAILABLE;
}

bool etcd::Response::is_grpc_timeout() const {
  return _error_code == grpc::StatusCode::DEADLINE_EXCEEDED;
}

std::string const& etcd::Response::action() const { return _action; }

int64_t etcd::Response::index() const { return _index; }

etcd::Value const& etcd::Response::value() const { return _value; }

etcd::Value const& etcd::Response::prev_value() const { return _prev_value; }

etcd::Values const& etcd::Response::values() const { return _values; }

etcd::Value const& etcd::Response::value(int index) const {
  return _values[index];
}

etcd::Keys const& etcd::Response::keys() const { return _keys; }

std::string const& etcd::Response::key(int index) const { return _keys[index]; }

int64_t etcd::Response::compact_revision() const { return _compact_revision; }

int64_t etcd::Response::watch_id() const { return _watch_id; }

std::string const& etcd::Response::lock_key() const { return _lock_key; }

std::string const& etcd::Response::name() const { return _name; }

std::vector<etcd::Event> const& etcd::Response::events() const {
  return this->_events;
}

std::chrono::microseconds const& etcd::Response::duration() const {
  return this->_duration;
}

uint64_t etcd::Response::cluster_id() const { return this->_cluster_id; }

uint64_t etcd::Response::member_id() const { return this->_member_id; }

uint64_t etcd::Response::raft_term() const { return this->_raft_term; }

std::vector<int64_t> const& etcd::Response::leases() const {
  return this->_leases;
}

std::vector<etcdv3::Member> const& etcd::Response::members() const {
  return this->_members;
}
