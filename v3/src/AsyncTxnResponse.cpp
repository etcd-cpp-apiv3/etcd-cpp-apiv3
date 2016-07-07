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
        values.insert(values.end(), response.values.begin(),response.values.end());
      }
    }
    else if(ResponseOp::ResponseCase::kResponsePut == resp.response_case())
    {
      auto put_resp = resp.response_put();
      if(put_resp.has_prev_kv())
      {
        prev_values.push_back(put_resp.prev_kv());
      }
    }
  }

  
}
