#include "etcd/v3/AsyncLeaseKeepAliveResponse.hpp"

#include <iostream>

#include "etcd/v3/action_constants.hpp"

using namespace std;
void etcdv3::AsyncLeaseKeepAliveResponse::ParseResponse(LeaseKeepAliveResponse& reply) {
  index = reply.header().revision();
  value.kvs.set_lease(reply.id());
  value.set_ttl(reply.ttl());
  return;
}
