#include "etcd/Response.hpp"
#include "v3/include/V3Response.hpp"

#include <iostream>


etcd::Response::Response(const etcdv3::V3Response& reply)
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

  _lock_key = reply.get_lock_key();
  _events = reply.get_events();
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

int etcd::Response::index() const
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

std::string const & etcd::Response::lock_key() const {
  return _lock_key;
}

std::vector<mvccpb::Event> const & etcd::Response::events() const {
  return this->_events;
};
