#include "etcd/v3/AsyncSetAction.hpp"

#include "etcd/v3/action_constants.hpp"
#include "etcd/v3/Transaction.hpp"

etcdv3::AsyncSetAction::AsyncSetAction(
    etcdv3::ActionParameters const &param, bool create)
  : etcdv3::Action(param) 
{
  etcdv3::Transaction transaction(parameters.key);
  isCreate = create;
  transaction.init_compare(CompareResult::EQUAL,
                           CompareTarget::VERSION);

  transaction.setup_basic_create_sequence(parameters.key, parameters.value, parameters.lease_id);

  if(isCreate)
  {
    transaction.setup_basic_failure_operation(parameters.key);
  }
  else
  {
    transaction.setup_set_failure_operation(parameters.key, parameters.value, parameters.lease_id);
  }
  response_reader = parameters.kv_stub->AsyncTxn(&context, *transaction.txn_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncTxnResponse etcdv3::AsyncSetAction::ParseResponse()
{

  AsyncTxnResponse txn_resp;
  txn_resp.set_action(isCreate? etcdv3::CREATE_ACTION : etcdv3::SET_ACTION);
  
  if(!status.ok())
  {
    txn_resp.set_error_code(status.error_code());
    txn_resp.set_error_message(status.error_message());
  }
  else
  { 
    txn_resp.ParseResponse(parameters.key, parameters.withPrefix, reply);

    if(!reply.succeeded() && isCreate)
    {
      txn_resp.set_error_code(etcdv3::ERROR_KEY_ALREADY_EXISTS);
      txn_resp.set_error_message("Key already exists");
    } 
  }
  return txn_resp;
}
