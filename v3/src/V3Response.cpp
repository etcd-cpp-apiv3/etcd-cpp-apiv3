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

void etcdv3::V3Response::set_action(std::string action)
{
  this->action = action;
}

std::vector<mvccpb::KeyValue> etcdv3::V3Response::get_kv_values()
{
  return values;
}

std::vector<mvccpb::KeyValue> etcdv3::V3Response::get_prev_kv_values()
{
  return prev_values;
}
