#ifndef __V3_ETCDV3KEYVALUE_HPP__
#define __V3_ETCDV3KEYVALUE_HPP__

#include "proto/kv.pb.h"


namespace etcdv3
{
  class KeyValue : public mvccpb::KeyValue
  {
  public:
    KeyValue();
    KeyValue(const mvccpb::KeyValue& from);
    void set_ttl(int ttl);
    int get_ttl() const;
  private:
    int ttl;
  };
}
#endif
