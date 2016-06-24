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

etcdv3::AsyncUpdateAction::AsyncUpdateAction(std::string const & key, std::string const & value, KV::Stub* stub_) 
{
  etcdv3::Transaction transaction(key);
  transaction.init_compare(Compare::CompareResult::Compare_CompareResult_GREATER,
		  	  	  	  	  	  Compare::CompareTarget::Compare_CompareTarget_VERSION);

  transaction.setup_basic_failure_operation(key);
  transaction.setup_compare_and_swap_sequence(value);

  response_reader = stub_->AsyncTxn(&context, transaction.txn_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncTxnResponse etcdv3::AsyncUpdateAction::ParseResponse()
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
    txn_resp.action = etcdv3::UPDATE_ACTION;
  }
  return txn_resp;
}
