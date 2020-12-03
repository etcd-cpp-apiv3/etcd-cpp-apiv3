#include "etcd/v3/AsyncLeaseRevokeResponse.hpp"
#include "etcd/v3/action_constants.hpp"

void etcdv3::AsyncLeaseRevokeResponse::ParseResponse(
    LeaseRevokeResponse &resp) {
  index = resp.header().revision();
}
