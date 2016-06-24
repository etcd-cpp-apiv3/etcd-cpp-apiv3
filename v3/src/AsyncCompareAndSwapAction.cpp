#include "v3/include/AsyncCompareAndSwapAction.hpp"
#include "v3/include/action_constants.hpp"
#include "v3/include/Transaction.hpp"

using etcdserverpb::Compare;
using etcdserverpb::RangeRequest;
using etcdserverpb::PutRequest;
using etcdserverpb::RequestOp;
using etcdserverpb::ResponseOp;
using etcdserverpb::TxnRequest;

etcdv3::AsyncCompareAndSwapAction::AsyncCompareAndSwapAction(std::string const & key, std::string const & value, std::string const & old_value, KV::Stub* stub_) 
{
  etcdv3::Transaction transaction(key);
  transaction.init_compare(old_value, Compare::CompareResult::Compare_CompareResult_EQUAL,
		  	  	  	  	  	  	  	  Compare::CompareTarget::Compare_CompareTarget_VALUE);

  transaction.setup_basic_failure_operation(key);
  transaction.setup_compare_and_swap_sequence(value);

  response_reader = stub_->AsyncTxn(&context, transaction.txn_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncCompareAndSwapAction::AsyncCompareAndSwapAction(std::string const & key, std::string const & value, int old_index, KV::Stub* stub_) 
{
  etcdv3::Transaction transaction(key);
  transaction.init_compare(old_index, Compare::CompareResult::Compare_CompareResult_EQUAL,
		  	  	  	  	  	  	  	  Compare::CompareTarget::Compare_CompareTarget_MOD);

  transaction.setup_basic_failure_operation(key);
  transaction.setup_compare_and_swap_sequence(value);

  response_reader = stub_->AsyncTxn(&context, transaction.txn_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncTxnResponse etcdv3::AsyncCompareAndSwapAction::ParseResponse()
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
    txn_resp.action = etcdv3::COMPARESWAP_ACTION;

    if(!reply.succeeded())
    {
      txn_resp.error_code=101;
      txn_resp.error_message="Compare failed";
    } 
  }
    
  return txn_resp;
}
