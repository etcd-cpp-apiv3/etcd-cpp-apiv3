#include "v3/include/AsyncUpdateAction.hpp"
#include "v3/include/AsyncRangeResponse.hpp"
#include "v3/include/action_constants.hpp"
#include "v3/include/Transaction.hpp"

using etcdserverpb::Compare;
using etcdserverpb::RangeRequest;
using etcdserverpb::PutRequest;
using etcdserverpb::RequestOp;
using etcdserverpb::ResponseOp;
using etcdserverpb::TxnRequest;

etcdv3::AsyncUpdateAction::AsyncUpdateAction(etcdv3::ActionParameters param)
  : etcdv3::Action(param) 
{
  etcdv3::Transaction transaction(parameters.key);
  transaction.init_compare(Compare::CompareResult::Compare_CompareResult_GREATER,
		  	  	  	  	  	   Compare::CompareTarget::Compare_CompareTarget_VERSION);

  transaction.setup_compare_and_swap_sequence(parameters.value, parameters.lease_id);

  response_reader = parameters.kv_stub->AsyncTxn(&context, transaction.txn_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncTxnResponse etcdv3::AsyncUpdateAction::ParseResponse()
{
  AsyncTxnResponse txn_resp;
  
  if(!status.ok())
  {
    txn_resp.set_error_code(status.error_code());
    txn_resp.set_error_message(status.error_message());
  }
  else
  { 
    if(reply.succeeded())
    {
      txn_resp.ParseResponse(parameters.key, parameters.withPrefix, reply);
      txn_resp.set_action(etcdv3::UPDATE_ACTION);
    }
    else
    {
      txn_resp.set_error_code(100);
      txn_resp.set_error_message("Key not found");
    }

  }
  return txn_resp;
}
