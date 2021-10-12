#include "etcd/v3/AsyncPutResponse.hpp"
#include "etcd/v3/action_constants.hpp"


void etcdv3::AsyncPutResponse::ParseResponse(PutResponse& resp)
{
  index = resp.header().revision();

  //get all previous values
  etcdv3::KeyValue kv; 
  kv.kvs.CopyFrom(resp.prev_kv());
  prev_value = kv;
}
