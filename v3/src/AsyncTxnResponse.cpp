#include "v3/include/AsyncTxnResponse.hpp"
#include "v3/include/AsyncRangeResponse.hpp"
#include "v3/include/AsyncDeleteRangeResponse.hpp"
#include "v3/include/action_constants.hpp"

using etcdserverpb::ResponseOp;

void etcdv3::AsyncTxnResponse::ParseResponse(std::string const& key, bool prefix, TxnResponse& reply)
{
  index = reply.header().revision();
  for(int index=0; index < reply.responses_size(); index++)
  {
    auto resp = reply.responses(index);
    if(ResponseOp::ResponseCase::kResponseRange == resp.response_case())
    {
      AsyncRangeResponse response;
      response.ParseResponse(*(resp.mutable_response_range()),prefix);
     
      error_code = response.get_error_code();
      error_message = response.get_error_message();
      
      values = response.get_values();
      value = response.get_value();
    }
    else if(ResponseOp::ResponseCase::kResponsePut == resp.response_case())
    {
      auto put_resp = resp.response_put();
      if(put_resp.has_prev_kv())
      {
        prev_value.kvs.CopyFrom(put_resp.prev_kv());
      }
    }
    else if(ResponseOp::ResponseCase::kResponseDeleteRange == resp.response_case())
    {
      AsyncDeleteRangeResponse response;
      response.ParseResponse(key,prefix,*(resp.mutable_response_delete_range()));

      prev_value.kvs.CopyFrom(response.get_prev_value().kvs);
     
      values = response.get_values();
      value = response.get_value();
    }
  }
}
