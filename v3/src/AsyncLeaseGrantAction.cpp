#include "v3/include/AsyncLeaseGrantAction.hpp"
#include "v3/include/action_constants.hpp"
#include "v3/include/Transaction.hpp"

using etcdserverpb::LeaseGrantRequest;

etcdv3::AsyncLeaseGrantAction::AsyncLeaseGrantAction(etcdv3::ActionParameters param)
  : etcdv3::Action(param)
{
  etcdv3::Transaction transaction;
  transaction.setup_lease_grant_operation(parameters.ttl);

  response_reader = parameters.lease_stub->AsyncLeaseGrant(&context, transaction.leasegrant_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
  
}


etcdv3::AsyncLeaseGrantResponse etcdv3::AsyncLeaseGrantAction::ParseResponse()
{
  AsyncLeaseGrantResponse lease_resp;
  if(!status.ok())
  {
    lease_resp.set_error_code(status.error_code());
    lease_resp.set_error_message(status.error_message());
  }
  else
  { 
    lease_resp.ParseResponse(reply);
  }
  return lease_resp;
}
