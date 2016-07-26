#include "v3/include/AsyncRangeResponse.hpp"
#include "v3/include/action_constants.hpp"


void etcdv3::AsyncRangeResponse::ParseResponse(RangeResponse& resp, bool prefix)
{
  action = etcdv3::GET_ACTION;
  index = resp.header().revision();
  if(resp.kvs_size() == 0)
  {
    error_code=100;
    error_message="Key not found";
    return;
  }
  else
  {
    for(int index=0; index < resp.kvs_size(); index++)
    {
      etcdv3::KeyValue kv;
      kv.kvs.CopyFrom(resp.kvs(index));
      values.push_back(kv); 
    }

    if(!prefix)
    {
      value = values[0];
      values.clear();
    }
  }
}
