#include <grpc/support/log.h>
#include "etcd/v3/Action.hpp"

etcdv3::Action::Action(etcdv3::ActionParameters params)
{
  parameters = params;
  if (!parameters.auth_token.empty()) {
    // use `authorization` as the key also works, see:
    //
    //  etcd/etcdserver/api/v3rpc/rpctypes/metadatafields.go
    context.AddMetadata("authorization", parameters.auth_token);
  }
  start_timepoint = std::chrono::high_resolution_clock::now();
}

etcdv3::ActionParameters::ActionParameters()
{
  withPrefix = false;
  revision = 0;
  old_revision = 0;
  lease_id = 0;
  ttl = 0;
  kv_stub = NULL;
  watch_stub = NULL;
  lease_stub = NULL;
}

void etcdv3::Action::waitForResponse() 
{
  void* got_tag;
  bool ok = false;    

  cq_.Next(&got_tag, &ok);
  GPR_ASSERT(got_tag == (void*)this);
}

const std::chrono::high_resolution_clock::time_point etcdv3::Action::startTimepoint() {
  return this->start_timepoint;
}
