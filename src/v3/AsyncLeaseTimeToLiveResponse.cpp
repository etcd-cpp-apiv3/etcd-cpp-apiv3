#include "etcd/v3/AsyncLeaseTimeToLiveResponse.hpp"
#include "etcd/v3/action_constants.hpp"

void etcdv3::AsyncLeaseTimeToLiveResponse::ParseResponse(
    LeaseTimeToLiveResponse &resp) {
  index = resp.header().revision();

  leaseinfo.set_lease(resp.id());
  leaseinfo.set_ttl(resp.ttl());
  leaseinfo.set_grantedttl(resp.grantedttl());

  for (int index = 0; index < resp.keys_size(); index++) {
    leaseinfo.add_key(resp.keys(index));
  }
}
