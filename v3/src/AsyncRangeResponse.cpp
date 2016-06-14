#include "v3/include/AsyncRangeResponse.hpp"

etcdv3::AsyncRangeResponse::AsyncRangeResponse(const etcdv3::AsyncRangeResponse& other) 
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

etcdv3::AsyncRangeResponse& etcdv3::AsyncRangeResponse::operator=(const etcdv3::AsyncRangeResponse& other) 
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

etcdv3::AsyncRangeResponse& etcdv3::AsyncRangeResponse::ParseResponse()
{
  index = reply.header().revision();
  if(!status.ok())
  {
    error_code = status.error_code();
    error_message = status.error_message();
  }
  else
  {

    if(reply.kvs_size() == 0)
    {
      error_code=100;
      error_message="Key not found";
    }

    for(int index=0; index < reply.kvs_size(); index++)
    {
      std::cout << "key: " << reply.kvs(index).key() << std::endl;
      std::cout << "value: " << reply.kvs(index).value()<< std::endl;
      values.push_back(reply.kvs(index)); 
    }
  }
  index = reply.header().revision();
  return *this;
}
