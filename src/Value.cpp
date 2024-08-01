#include <cstdint>
#include <iomanip>

#include "etcd/Value.hpp"
#include "etcd/v3/KeyValue.hpp"

etcd::Value::Value()
    : dir(false), created(0), modified(0), _version(0), _ttl(0), leaseId(0) {}

etcd::Value::Value(etcdv3::KeyValue const& kv) {
  dir = false;
  _key = kv.kvs.key();
  value = kv.kvs.value();
  created = kv.kvs.create_revision();
  modified = kv.kvs.mod_revision();
  _version = kv.kvs.version();
  leaseId = kv.kvs.lease();
  _ttl = kv.get_ttl();
}

etcd::Value::Value(mvccpb::KeyValue const& kv) {
  dir = false;
  _key = kv.key();
  value = kv.value();
  created = kv.create_revision();
  modified = kv.mod_revision();
  _version = kv.version();
  leaseId = kv.lease();
  _ttl = -1;
}

std::string const& etcd::Value::key() const { return _key; }

bool etcd::Value::is_dir() const { return dir; }

std::string const& etcd::Value::as_string() const { return value; }

int64_t etcd::Value::created_index() const { return created; }

int64_t etcd::Value::modified_index() const { return modified; }

int64_t etcd::Value::version() const { return _version; }

int etcd::Value::ttl() const { return _ttl; }

int64_t etcd::Value::lease() const { return leaseId; }

std::ostream& etcd::operator<<(std::ostream& os, const etcd::Value& value) {
  os << "Event: {";
  os << "Key: " << value.key() << ", ";
  os << "Value: " << value.as_string() << ", ";
  os << "Created: " << value.created_index() << ", ";
  os << "Modified: " << value.modified_index() << ", ";
  os << "Version: " << value.version() << ", ";
  os << "TTL: " << value.ttl() << ", ";
  os << "Lease: " << value.lease() << ", ";
  os << "}";
  return os;
}

etcd::Event::Event(mvccpb::Event const& event) {
  _has_kv = event.has_kv();
  _has_prev_kv = event.has_prev_kv();
  if (_has_kv) {
    _kv = Value(event.kv());
  }
  if (_has_prev_kv) {
    _prev_kv = Value(event.prev_kv());
  }
  if (event.type() == mvccpb::Event::PUT) {
    event_type_ = EventType::PUT;
  } else if (event.type() == mvccpb::Event::DELETE_) {
    event_type_ = EventType::DELETE_;
  } else {
    event_type_ = EventType::INVALID;
  }
}

enum etcd::Event::EventType etcd::Event::event_type() const {
  return event_type_;
}

bool etcd::Event::has_kv() const { return _has_kv; }

bool etcd::Event::has_prev_kv() const { return _has_prev_kv; }

const etcd::Value& etcd::Event::kv() const { return _kv; }

const etcd::Value& etcd::Event::prev_kv() const { return _prev_kv; }

std::ostream& etcd::operator<<(std::ostream& os,
                               const etcd::Event::EventType& value) {
  switch (value) {
  case etcd::Event::EventType::PUT:
    os << "PUT";
    break;
  case etcd::Event::EventType::DELETE_:
    os << "DELETE";
    break;
  case etcd::Event::EventType::INVALID:
    os << "INVALID";
    break;
  }
  return os;
}

std::ostream& etcd::operator<<(std::ostream& os, const etcd::Event& event) {
  os << "Event type: " << event.event_type();
  if (event.has_kv()) {
    os << ", KV: " << event.kv();
  }
  if (event.has_prev_kv()) {
    os << ", Prev KV: " << event.prev_kv();
  }
  return os;
}
