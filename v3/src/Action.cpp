#include "v3/include/Action.hpp"

void etcdv3::Action::waitForResponse() 
{
  void* got_tag;
  bool ok = false;    

  cq_.Next(&got_tag, &ok);
  GPR_ASSERT(got_tag == (void*)this);
}

void etcdv3::Actionv2::waitForResponse() 
{
  void* got_tag;
  bool ok = false;    

  cq_.Next(&got_tag, &ok);
  GPR_ASSERT(got_tag == (void*)this);
}

etcdv3::Actionv2::Actionv2(etcdv3::ActionParameters params)
{
  parameters = params;
}
