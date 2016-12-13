#include <grpc/support/log.h>
#include "v3/include/Action.hpp"

etcdv3::Action::Action(etcdv3::ActionParameters params)
{
  parameters = params;
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

