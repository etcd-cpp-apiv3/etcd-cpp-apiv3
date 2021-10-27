#include "etcd/Response.hpp"
#include "etcd/v3/V3Response.hpp"

#include <iostream>

etcd::Response::Response(const etcdv3::V3Response& reply, std::chrono::microseconds const& duration)
{
  _index = reply.get_index();
  _action = reply.get_action();
  _error_code = reply.get_error_code();
  _error_message = reply.get_error_message();
  if(reply.has_values())
  {
    auto val = reply.get_values();
    for(unsigned int index = 0; index < val.size(); index++)
    {
      _values.push_back(Value(val[index]));
      _keys.push_back(val[index].kvs.key());
    }
  }
  else
  {
    _value = Value(reply.get_value());
  }
  _prev_value = Value(reply.get_prev_value());

  _compact_revision = reply.get_compact_revision();
  _lock_key = reply.get_lock_key();
  _name = reply.get_name();

  for (auto const &ev: reply.get_events()) {
    _events.emplace_back(etcd::Event(ev));
  }

  // duration
  _duration = duration;
}


etcd::Response::Response()
  : _error_code(0),
    _index(0)
{
}

etcd::Response::Response(int error_code, char const * error_message)
  : _error_code(error_code),
    _error_message(error_message),
    _index(0)
{
}

int etcd::Response::error_code() const
{
  return _error_code;
}

std::string const & etcd::Response::error_message() const
{
  return _error_message;
}

int64_t etcd::Response::index() const
{
  return _index;
}

std::string const & etcd::Response::action() const
{
  return _action;
}

bool etcd::Response::is_ok() const
{
  return error_code() == 0;
}

bool etcd::Response::is_network_unavailable() const
{
  return error_code() == ::grpc::StatusCode::UNAVAILABLE;
}

etcd::Value const & etcd::Response::value() const
{
  return _value;
}

etcd::Value const & etcd::Response::prev_value() const
{
  return _prev_value;
}

etcd::Values const & etcd::Response::values() const
{
  return _values;
}

etcd::Value const & etcd::Response::value(int index) const
{
  return _values[index];
}

etcd::Keys const & etcd::Response::keys() const
{
  return _keys;
}

std::string const & etcd::Response::key(int index) const
{
  return _keys[index];
}

int64_t etcd::Response::compact_revision() const
{
  return _compact_revision;
}

std::string const & etcd::Response::lock_key() const {
  return _lock_key;
}

std::string const & etcd::Response::name() const {
  return _name;
}

std::vector<etcd::Event> const & etcd::Response::events() const {
  return this->_events;
}

std::chrono::microseconds const& etcd::Response::duration() const {
  return this->_duration;
}
