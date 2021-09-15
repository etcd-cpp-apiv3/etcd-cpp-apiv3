#include "etcd/v3/AsyncRangeResponse.hpp"
#include "etcd/v3/action_constants.hpp"


void etcdv3::AsyncRangeResponse::ParseResponse(RangeResponse& resp, bool prefix)
{
  index = resp.header().revision();
  if(resp.kvs_size() == 0 && !prefix)
  {
    error_code = etcdv3::ERROR_KEY_NOT_FOUND;
    error_message = "Key not found";
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
