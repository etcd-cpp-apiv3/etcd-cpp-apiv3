#include "v3/include/AsyncTxnResponse.hpp"

using etcdserverpb::ResponseOp;


etcdv3::AsyncTxnResponse::AsyncTxnResponse(const etcdv3::AsyncTxnResponse& other) 
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

etcdv3::AsyncTxnResponse& etcdv3::AsyncTxnResponse::operator=(const etcdv3::AsyncTxnResponse& other) 
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

etcdv3::AsyncTxnResponse& etcdv3::AsyncTxnResponse::ParseResponse()
{
  for(int index=0; index < reply.responses_size(); index++)
  {
    auto resp = reply.responses(index);
    if(ResponseOp::ResponseCase::kResponseRange == resp.response_case())
    {
      if(resp.response_range().kvs_size())
      {
        if(!resp.response_range().more())
        {
          if(!values.empty())
          {
            prev_value = values[0];
          }
          values.clear();
          values.push_back(resp.response_range().kvs(0));
        }
      }
      else
      {
        error_code=100;
        error_message="Key not found";
      }
    }
    else if(ResponseOp::ResponseCase::kResponsePut == resp.response_case())
    {
      //do nothing for now.
    }
    else
    {
      //do nothing for now.
    }
  }




  if(action == "create")
  {

    if(!reply.succeeded())
    {
      error_code=105;
      error_message="Key already exists";
    }
  }
  else if(action == "compareAndSwap")
  {
    if(!reply.succeeded())
    {
      error_code=101;
      error_message="Compare failed";
    }
  }
  return *this;
}
