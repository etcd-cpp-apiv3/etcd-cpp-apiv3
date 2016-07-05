#include "v3/include/AsyncTxnResponse.hpp"
#include "v3/include/AsyncRangeResponse.hpp"
#include "v3/include/action_constants.hpp"

using etcdserverpb::ResponseOp;

etcdv3::AsyncTxnResponse::AsyncTxnResponse(TxnResponse& resp) 
{
  reply = resp;
}

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

void etcdv3::AsyncTxnResponse::ParseResponse()
{
  index = reply.header().revision();
  std::vector<mvccpb::KeyValue> range_kvs;
  std::vector<mvccpb::KeyValue> prev_range_kvs;
  for(int index=0; index < reply.responses_size(); index++)
  {
    auto resp = reply.responses(index);
    if(ResponseOp::ResponseCase::kResponseRange == resp.response_case())
    {
      AsyncRangeResponse response(*(resp.mutable_response_range()));
      response.ParseResponse();
     
      error_code = response.error_code;
      error_message = response.error_message;

      if(!response.values.empty())
      {
        prev_range_kvs=range_kvs;
        range_kvs = response.values;
      }
    }
    else if(ResponseOp::ResponseCase::kResponseDeleteRange == resp.response_case())
    {
      std::cout << "number of deleted keys: " << resp.response_delete_range().deleted() <<std::endl;
    }
  }
  prev_values = prev_range_kvs;
  values = range_kvs; 
  
}
