#include "v3/include/V3Response.hpp"
#include "v3/include/action_constants.hpp"

void etcdv3::V3Response::set_error_code(int code) 
{
  error_code = code;
}

void etcdv3::V3Response::set_error_message(std::string msg)
{
  error_message = msg;
}

int etcdv3::V3Response::get_index() const
{
  return index;
}

std::string const & etcdv3::V3Response::get_action() const
{
  return action;
}

int etcdv3::V3Response::get_error_code() const
{
  return error_code;
}

std::string const & etcdv3::V3Response::get_error_message() const
{
  return error_message;
}

void etcdv3::V3Response::set_action(std::string action)
{
  this->action = action;
}

std::vector<etcdv3::KeyValue> const & etcdv3::V3Response::get_values() const
{
  return values;
}

std::vector<etcdv3::KeyValue> const & etcdv3::V3Response::get_prev_values() const
{
  return prev_values;
}

etcdv3::KeyValue const & etcdv3::V3Response::get_value() const
{
  return value;
}

etcdv3::KeyValue const & etcdv3::V3Response::get_prev_value() const
{
  return prev_value;
}

bool etcdv3::V3Response::has_values() const
{
  return values.size() > 0;
}
