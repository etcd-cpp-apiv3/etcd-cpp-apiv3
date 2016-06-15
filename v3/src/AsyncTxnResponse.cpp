#include "v3/include/AsyncTxnResponse.hpp"
#include "v3/include/AsyncRangeResponse.hpp"

using etcdserverpb::ResponseOp;


etcdv3::AsyncTxnResponse::AsyncTxnResponse(const etcdv3::AsyncTxnResponse& other) 
{
  error_code = other.error_code;
  error_message = other.error_message;
  index = other.index;
  action = other.action;
  values = other.values;
  prev_values = other.prev_values;

}

etcdv3::AsyncTxnResponse& etcdv3::AsyncTxnResponse::operator=(const etcdv3::AsyncTxnResponse& other) 
{
  error_code = other.error_code;
  error_message = other.error_message;
  index = other.index;
  action = other.action;
  values = other.values;
  prev_values = other.prev_values;
  return *this;
}

etcdv3::AsyncTxnResponse& etcdv3::AsyncTxnResponse::ParseResponse()
{

  index = reply.header().revision();
  if(!status.ok())
  {
    error_code = status.error_code();
    error_message = status.error_message();
  }
  else
  {
    std::vector<mvccpb::KeyValue> range_kvs;
    std::vector<mvccpb::KeyValue> prev_range_kvs;
    for(int index=0; index < reply.responses_size(); index++)
    {
      auto resp = reply.responses(index);
      if(ResponseOp::ResponseCase::kResponseRange == resp.response_case())
      {
        AsyncRangeResponse response;
        response.reply = resp.response_range();
        auto v3resp = response.ParseResponse();
     
        error_code = v3resp.error_code;
        error_message = v3resp.error_message;

        if(!v3resp.values.empty())
        {
          prev_range_kvs=range_kvs;
          range_kvs = v3resp.values;
        }
      }
      else if(ResponseOp::ResponseCase::kResponseDeleteRange == resp.response_case())
      {
        std::cout << "deleted keys: " << resp.response_delete_range().deleted() << std:: endl;
      }
    }

    if(!reply.succeeded())
    { 
      if(action == "create")
      {
        error_code=105;
        error_message="Key already exists";
      }
      else if(action == "compareAndSwap")
      {
        error_code=101;
        error_message="Compare failed";
      }
    }

    prev_values = prev_range_kvs;

    values = range_kvs;    

    if(action == "delete")
    {
      prev_values = values;
    }

  }       
  return *this;
}
