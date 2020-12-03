#ifndef __V3_ETCDV3LEASEINFO_HPP__
#define __V3_ETCDV3LEASEINFO_HPP__

#include "proto/kv.pb.h"

namespace etcdv3 {
class LeaseInfo {
 public:
  LeaseInfo();
  // mvccpb::KeyValue kvs;
  void set_lease(int64_t leaseid);
  void set_ttl(int ttl);
  void set_grantedttl(int ttl);
  void add_key(std::string key);
  int64_t get_lease() const;
  int get_ttl() const;
  int get_grantedttl() const;
  std::vector<std::string> get_keys() const;

 private:
  int64_t leaseid_;
  int ttl_;
  int grantedttl_;
  std::vector<std::string> keys_;
};
}  // namespace etcdv3
#endif
