#include "etcd/v3/AsyncDeleteResponse.hpp"
#include "etcd/v3/action_constants.hpp"


void etcdv3::AsyncDeleteResponse::ParseResponse(std::string const& key, bool prefix, DeleteRangeResponse& resp)
{
  index = resp.header().revision();

  if(resp.prev_kvs_size() == 0)
  {
    error_code = etcdv3::ERROR_KEY_NOT_FOUND;
    error_message = "Key not found";
  }
  else
  {
    //get all previous values
    for(int cnt=0; cnt < resp.prev_kvs_size(); cnt++)
    {
      etcdv3::KeyValue kv;
      kv.kvs.CopyFrom(resp.prev_kvs(cnt));
      values.push_back(kv);
    }

    if(!prefix)
    {
      prev_value = values[0];
      value = values[0];
      value.kvs.clear_value();
      values.clear();
    }
  }
}
