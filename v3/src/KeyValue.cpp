#include "v3/include/KeyValue.hpp"

etcdv3::KeyValue::KeyValue()
{
  ttl = 0;
}

etcdv3::KeyValue::KeyValue(const mvccpb::KeyValue& from) 
  : mvccpb::KeyValue(from)
{
  ttl =0;
}

void etcdv3::KeyValue::set_ttl(int ttl) 
{
  this->ttl = ttl;
}

int etcdv3::KeyValue::get_ttl() const
{
  return ttl;
}
