#include "v3/include/AsyncRangeResponse.hpp"

etcdv3::AsyncRangeResponse::AsyncRangeResponse(const etcdv3::AsyncRangeResponse& other) 
{
  error_code = other.error_code;
  error_message = other.error_message;
  index = other.index;
  action = other.action;
  values = other.values;
}

etcdv3::AsyncRangeResponse& etcdv3::AsyncRangeResponse::operator=(const etcdv3::AsyncRangeResponse& other) 
{
  error_code = other.error_code;
  error_message = other.error_message;
  index = other.index;
  action = other.action;
  values = other.values;
  return *this;
}

etcdv3::AsyncRangeResponse& etcdv3::AsyncRangeResponse::ParseResponse()
{
  action = "get";
  
  if(reply.kvs_size())
  {
    if(reply.more())
    {
      for(int index=0; reply.more(); index++)
      {
        values.push_back(reply.kvs(index));
      }
    }
    else
    {
      values.push_back(reply.kvs(0));
    }
  }
  else
  {
    error_code=100;
    error_message="Key not found";
  }

  return *this;
}


