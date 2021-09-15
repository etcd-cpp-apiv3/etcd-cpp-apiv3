#include "etcd/v3/AsyncCompareAndSwapAction.hpp"

#include "etcd/v3/action_constants.hpp"
#include "etcd/v3/Transaction.hpp"

using etcdserverpb::RangeRequest;
using etcdserverpb::PutRequest;
using etcdserverpb::RequestOp;
using etcdserverpb::ResponseOp;
using etcdserverpb::TxnRequest;

etcdv3::AsyncCompareAndSwapAction::AsyncCompareAndSwapAction(
    etcdv3::ActionParameters const &param, etcdv3::AtomicityType type)
  : etcdv3::Action(param)  
{
  etcdv3::Transaction transaction(parameters.key);
  if(type == etcdv3::AtomicityType::PREV_VALUE)
  {
    transaction.init_compare(parameters.old_value,
                             CompareResult::EQUAL,
                             CompareTarget::VALUE);
  }
  else if (type == etcdv3::AtomicityType::PREV_INDEX)
  {
    transaction.init_compare(parameters.old_revision,
                             CompareResult::EQUAL,
                             CompareTarget::MOD);
  }

  transaction.setup_basic_failure_operation(parameters.key);
  transaction.setup_compare_and_swap_sequence(parameters.value, parameters.lease_id);

  response_reader = parameters.kv_stub->AsyncTxn(&context, *transaction.txn_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncTxnResponse etcdv3::AsyncCompareAndSwapAction::ParseResponse()
{
  AsyncTxnResponse txn_resp;
  txn_resp.set_action(etcdv3::COMPARESWAP_ACTION);

  if(!status.ok())
  {
    txn_resp.set_error_code(status.error_code());
    txn_resp.set_error_message(status.error_message());
  }
  else
  { 
    txn_resp.ParseResponse(parameters.key, parameters.withPrefix, reply);

    //if there is an error code returned by parseResponse, we must 
    //not overwrite it.
    if(!reply.succeeded() && !txn_resp.get_error_code())
    {
      txn_resp.set_error_code(ERROR_COMPARE_FAILED);
      txn_resp.set_error_message("Compare failed");
    } 
  }
    
  return txn_resp;
}
