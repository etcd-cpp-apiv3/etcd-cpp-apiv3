#include "v3/include/AsyncPutResponse.hpp"

using etcdserverpb::PutRequest;
using etcdserverpb::PutRequest;

etcdv3::AsyncPutResponse::AsyncPutResponse(const etcdv3::AsyncPutResponse& other) 
{
  error_code = other.error_code;
  error_message = other.error_message;
  index = other.index;
  action = other.action;
  values = other.values;
  prev_value.set_key(other.prev_value.key());
  prev_value.set_value(other.prev_value.value());
  prev_value.set_create_revision(other.prev_value.create_revision());
  prev_value.set_mod_revision(other.prev_value.mod_revision());

}

etcdv3::AsyncPutResponse& etcdv3::AsyncPutResponse::operator=(const etcdv3::AsyncPutResponse& other) 
{
  error_code = other.error_code;
  error_message = other.error_message;
  index = other.index;
  action = other.action;
  values = other.values;
  prev_value.set_key(other.prev_value.key());
  prev_value.set_value(other.prev_value.value());
  prev_value.set_create_revision(other.prev_value.create_revision());
  prev_value.set_mod_revision(other.prev_value.mod_revision());
  return *this;
}

etcdv3::AsyncPutResponse& etcdv3::AsyncPutResponse::ParseResponse()
{
  action = "set";
  return *this;
}
