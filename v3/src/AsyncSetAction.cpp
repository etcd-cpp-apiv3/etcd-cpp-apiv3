#include "v3/include/AsyncSetAction.hpp"
#include "v3/include/AsyncRangeResponse.hpp"
#include "v3/include/action_constants.hpp"
#include "v3/include/Transaction.hpp"

using etcdserverpb::Compare;
using etcdserverpb::RangeRequest;
using etcdserverpb::PutRequest;
using etcdserverpb::RequestOp;
using etcdserverpb::ResponseOp;
using etcdserverpb::TxnRequest;

etcdv3::AsyncSetAction::AsyncSetAction(etcdv3::ActionParameters param, bool create)
  : etcdv3::Actionv2(param) 
{
  etcdv3::Transaction transaction(parameters.key);
  isCreate = create;
  transaction.init_compare(Compare::CompareResult::Compare_CompareResult_EQUAL,
		  	  	  	  	  	  Compare::CompareTarget::Compare_CompareTarget_VERSION);

  transaction.setup_basic_create_sequence(parameters.key, parameters.value);
  if(isCreate)
  {

    transaction.setup_basic_failure_operation(parameters.key);
    //transaction.setup_basic_create_sequence(parameters.key, parameters.value);
  }
  else
  {
    transaction.setup_set_failure_operation(parameters.key, parameters.value);
    //transaction.setup_basic_create_sequence(parameters.key, parameters.value);
  }
  response_reader = parameters.kv_stub->AsyncTxn(&context, transaction.txn_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncTxnResponse etcdv3::AsyncSetAction::ParseResponse()
{

  AsyncTxnResponse txn_resp(reply);
  
  if(!status.ok())
  {
    txn_resp.error_code = status.error_code();
    txn_resp.error_message = status.error_message();
  }
  else
  { 
    txn_resp.ParseResponse();
    txn_resp.action = isCreate? etcdv3::CREATE_ACTION:etcdv3::SET_ACTION;

    if(!reply.succeeded() && txn_resp.action == etcdv3::CREATE_ACTION)
    {
      txn_resp.error_code=105;
      txn_resp.error_message="Key already exists";
    } 
  }
  return txn_resp;
}
