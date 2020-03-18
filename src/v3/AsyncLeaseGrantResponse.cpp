#include "etcd/v3/AsyncLeaseGrantResponse.hpp"
#include "etcd/v3/action_constants.hpp"


void etcdv3::AsyncLeaseGrantResponse::ParseResponse(LeaseGrantResponse& resp)
{
  index = resp.header().revision();
  value.kvs.set_lease(resp.id());
  value.set_ttl(resp.ttl());
  error_message = resp.error();
}
