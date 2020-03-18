#include "etcd/v3/KeyValue.hpp"

etcdv3::KeyValue::KeyValue()
{
  ttl = 0;
}

void etcdv3::KeyValue::set_ttl(int ttl) 
{
  this->ttl = ttl;
}

int etcdv3::KeyValue::get_ttl() const
{
  return ttl;
}
