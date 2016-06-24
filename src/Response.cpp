#include "etcd/Response.hpp"

#include <iostream>


etcd::Response::Response(const etcdv3::V3Response& reply)
{
  _index = reply.index;
  _error_code = reply.error_code;
  _error_message = reply.error_message;
  _action = reply.action;
  int size = reply.values.size();
  if(size > 1)
  {
    for(int index = 0; index < size; index++)
    {
      _values.push_back(Value(reply.values[index]));
      _keys.push_back(reply.values[index].key());
    }
  }
  else if(size == 1)
  {
    _value = Value(reply.values[0]);
  }
  
  if(reply.prev_values.size() == 1)
  {
    _prev_value = Value(reply.prev_values[0]);
  }
}


etcd::Response::Response()
  : _error_code(0),
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
