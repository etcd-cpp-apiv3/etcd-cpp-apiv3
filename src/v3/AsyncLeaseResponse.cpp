#include "etcd/v3/AsyncLeaseResponse.hpp"
#include "etcd/v3/action_constants.hpp"


void etcdv3::AsyncLeaseGrantResponse::ParseResponse(LeaseGrantResponse& resp) {
  index = resp.header().revision();
  value.kvs.set_lease(resp.id());
  value.set_ttl(resp.ttl());
  error_message = resp.error();
}

void etcdv3::AsyncLeaseRevokeResponse::ParseResponse(LeaseRevokeResponse& resp) {
  index = resp.header().revision();
}

void etcdv3::AsyncLeaseKeepAliveResponse::ParseResponse(LeaseKeepAliveResponse& resp) {
  index = resp.header().revision();
  value.kvs.set_lease(resp.id());
  value.set_ttl(resp.ttl());
}

void etcdv3::AsyncLeaseTimeToLiveResponse::ParseResponse(LeaseTimeToLiveResponse& resp) {
  index = resp.header().revision();
  value.kvs.set_lease(resp.id());
  value.set_ttl(resp.ttl());
  // FIXME: unsupported: fields "grantedTTL" and "keys"
}

void etcdv3::AsyncLeaseLeasesResponse::ParseResponse(LeaseLeasesResponse& resp) {
  index = resp.header().revision();
  for (auto lease : resp.leases()) {
    leases.emplace_back(lease.id());
  }
}
