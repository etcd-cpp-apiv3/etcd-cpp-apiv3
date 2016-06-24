#include "v3/include/AsyncRangeResponse.hpp"
#include "v3/include/action_constants.hpp"

etcdv3::AsyncRangeResponse::AsyncRangeResponse(RangeResponse& resp) 
{
  reply = resp;
}

etcdv3::AsyncRangeResponse::AsyncRangeResponse(const etcdv3::AsyncRangeResponse& other) 
{
  error_code = other.error_code;
  error_message = other.error_message;
  index = other.index;
  action = other.action;
  values = other.values;
  prev_values = other.prev_values;

}

etcdv3::AsyncRangeResponse& etcdv3::AsyncRangeResponse::operator=(const etcdv3::AsyncRangeResponse& other) 
{
  error_code = other.error_code;
  error_message = other.error_message;
  index = other.index;
  action = other.action;
  values = other.values;
  prev_values = other.prev_values;
  return *this;
}

void etcdv3::AsyncRangeResponse::ParseResponse()
{
  index = reply.header().revision();
  if(reply.kvs_size() == 0)
  {
    error_code=100;
    error_message="Key not found";
  }

  for(int index=0; index < reply.kvs_size(); index++)
  {
    values.push_back(reply.kvs(index)); 
  }
}
