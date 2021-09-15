#include "etcd/v3/AsyncUpdateAction.hpp"

#include "etcd/v3/action_constants.hpp"
#include "etcd/v3/AsyncRangeResponse.hpp"
#include "etcd/v3/Transaction.hpp"

using etcdserverpb::RangeRequest;
using etcdserverpb::PutRequest;
using etcdserverpb::RequestOp;
using etcdserverpb::ResponseOp;
using etcdserverpb::TxnRequest;

etcdv3::AsyncUpdateAction::AsyncUpdateAction(
    etcdv3::ActionParameters const &param)
  : etcdv3::Action(param) 
{
  etcdv3::Transaction transaction(parameters.key);
  transaction.init_compare(CompareResult::GREATER,
                           CompareTarget::VERSION);

  transaction.setup_compare_and_swap_sequence(parameters.value, parameters.lease_id);

  response_reader = parameters.kv_stub->AsyncTxn(&context, *transaction.txn_request, &cq_);
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
      txn_resp.set_error_code(etcdv3::ERROR_KEY_NOT_FOUND);
      txn_resp.set_error_message("Key not found");
    }
  }
  return txn_resp;
}
