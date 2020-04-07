#include "v3/include/action_constants.hpp"
#include "v3/include/AsyncTxnAction.hpp"
#include "v3/include/Transaction.hpp"


etcdv3::AsyncTxnAction::AsyncTxnAction(etcdv3::ActionParameters param, etcdv3::Transaction const &tx)
  : etcdv3::Action(param)
{
    response_reader = parameters.kv_stub->AsyncTxn(&context, tx.txn_request, &cq_);
    response_reader->Finish(&reply, &status, (void *)this);
}

etcdv3::AsyncTxnResponse etcdv3::AsyncTxnAction::ParseResponse()
{
  AsyncTxnResponse txn_resp;
  
  if(!status.ok())
  {
    txn_resp.set_error_code(status.error_code());
    txn_resp.set_error_message(status.error_message());
  }
  else
  { 
    txn_resp.ParseResponse(parameters.key, parameters.withPrefix, reply);
    txn_resp.set_action(etcdv3::TXN_ACTION);

    //if there is an error code returned by parseResponse, we must 
    //not overwrite it.
    if(!reply.succeeded() && !txn_resp.get_error_code())
    {
      txn_resp.set_error_code(101);
      txn_resp.set_error_message("compare failed");
    } 
  }
    
  return txn_resp;
}
