#include "v3/include/AsyncPutResponse.hpp"
#include "v3/include/Utils.hpp"

#include <iostream>

using etcdserverpb::PutRequest;
using etcdserverpb::PutRequest;

etcdv3::AsyncPutResponse::AsyncPutResponse(const etcdv3::AsyncPutResponse& other) 
{
  error_code = other.error_code;
  error_message = other.error_message;
  index = other.index;
  action = other.action;
  values = other.values;
  prev_values= other.prev_values;

}

etcdv3::AsyncPutResponse& etcdv3::AsyncPutResponse::operator=(const etcdv3::AsyncPutResponse& other) 
{
  error_code = other.error_code;
  error_message = other.error_message;
  index = other.index;
  action = other.action;
  values = other.values;
  prev_values= other.prev_values;
  return *this;
}

etcdv3::AsyncPutResponse& etcdv3::AsyncPutResponse::ParseResponse()
{
  etcdv3::AsyncRangeResponse* resp = etcdv3::Utils::getKey(key, *client);
  if(resp->reply.kvs_size())
  {
    values.push_back(resp->reply.kvs(0));
    index = resp->reply.kvs(0).create_revision();
  }
  else
	  index = resp->reply.header().revision();

  return *this;
}
