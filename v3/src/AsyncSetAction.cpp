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

etcdv3::AsyncSetAction::AsyncSetAction(std::string const & key, std::string const & value, KV::Stub* stub_, bool create) 
{
  etcdv3::Transaction transaction(key);
  isCreate = create;
  if(create)
  {
    transaction.init_compare(Compare::CompareResult::Compare_CompareResult_EQUAL,
		  	  	  	  	  	  Compare::CompareTarget::Compare_CompareTarget_VERSION);
    transaction.setup_basic_failure_operation(key);
    transaction.setup_basic_create_sequence(key, value);
  }
  else
  {
    transaction.init_compare(Compare::CompareResult::Compare_CompareResult_EQUAL,
		  	  	  	  	  	  Compare::CompareTarget::Compare_CompareTarget_VERSION);
    transaction.setup_set_failure_operation(key, value);
    transaction.setup_basic_create_sequence(key, value);
  }
  response_reader = stub_->AsyncTxn(&context, transaction.txn_request, &cq_);
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
