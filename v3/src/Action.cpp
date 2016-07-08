#include "v3/include/Action.hpp"

etcdv3::Action::Action(etcdv3::ActionParameters params)
{
  parameters = params;
}

void etcdv3::Action::waitForResponse() 
{
  void* got_tag;
  bool ok = false;    

  cq_.Next(&got_tag, &ok);
  GPR_ASSERT(got_tag == (void*)this);
}

