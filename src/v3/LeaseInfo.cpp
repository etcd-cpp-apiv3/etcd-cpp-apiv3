#include "etcd/v3/LeaseInfo.hpp"

etcdv3::LeaseInfo::LeaseInfo() {
  leaseid_ = 0;
  ttl_ = 0;
  grantedttl_ = 0;
  keys_.clear();
}

void etcdv3::LeaseInfo::set_lease(int64_t id) { leaseid_ = id; }
void etcdv3::LeaseInfo::set_ttl(int ttl) { ttl_ = ttl; }
void etcdv3::LeaseInfo::set_grantedttl(int ttl) { grantedttl_ = ttl; }
void etcdv3::LeaseInfo::add_key(std::string key) { keys_.push_back(key); }

int64_t etcdv3::LeaseInfo::get_lease() const { return leaseid_; }
int etcdv3::LeaseInfo::get_ttl() const { return ttl_; }
int etcdv3::LeaseInfo::get_grantedttl() const { return grantedttl_; }
std::vector<std::string> etcdv3::LeaseInfo::get_keys() const { return keys_; }
