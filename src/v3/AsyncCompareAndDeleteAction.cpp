#include "etcd/v3/AsyncCompareAndDeleteAction.hpp"
#include "etcd/v3/action_constants.hpp"
#include "etcd/v3/Transaction.hpp"

using etcdserverpb::Compare;
using etcdserverpb::RangeRequest;
using etcdserverpb::PutRequest;
using etcdserverpb::RequestOp;
using etcdserverpb::ResponseOp;
using etcdserverpb::TxnRequest;

etcdv3::AsyncCompareAndDeleteAction::AsyncCompareAndDeleteAction(etcdv3::ActionParameters param, etcdv3::Atomicity_Type type) 
  :etcdv3::Action(param)
{
  etcdv3::Transaction transaction(parameters.key);
  if(type == etcdv3::Atomicity_Type::PREV_VALUE)
  {
    transaction.init_compare(parameters.old_value, Compare::CompareResult::Compare_CompareResult_EQUAL,
		    Compare::CompareTarget::Compare_CompareTarget_VALUE);
  }
  else if (type == etcdv3::Atomicity_Type::PREV_INDEX)
  {
    transaction.init_compare(parameters.old_revision, Compare::CompareResult::Compare_CompareResult_EQUAL,
								  Compare::CompareTarget::Compare_CompareTarget_MOD);
  }

  transaction.setup_compare_and_delete_operation(parameters.key);
  transaction.setup_basic_failure_operation(parameters.key);

  response_reader = parameters.kv_stub->AsyncTxn(&context, *transaction.txn_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncTxnResponse etcdv3::AsyncCompareAndDeleteAction::ParseResponse()
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
    txn_resp.set_action(etcdv3::COMPAREDELETE_ACTION);

    if(!reply.succeeded())
    {
      txn_resp.set_error_code(101);
      txn_resp.set_error_message("Compare failed");
    } 
  }
    
  return txn_resp;
}
