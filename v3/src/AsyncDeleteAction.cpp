#include "v3/include/AsyncDeleteAction.hpp"
#include "v3/include/action_constants.hpp"
#include "v3/include/Transaction.hpp"

using etcdserverpb::Compare;

etcdv3::AsyncDeleteAction::AsyncDeleteAction(std::string const & key, KV::Stub* stub_, bool recursive) 
{
  etcdv3::Transaction transaction(key);
  transaction.init_compare(Compare::CompareResult::Compare_CompareResult_GREATER,
							  Compare::CompareTarget::Compare_CompareTarget_VERSION);
  std::string range_end(key); 
  if(recursive)
  {
    int ascii = (int)range_end[range_end.length()-1];
    range_end.back() = ascii+1;
  }

  transaction.setup_delete_sequence(key, range_end, recursive);
  transaction.setup_delete_failure_operation(key, range_end, recursive);

  response_reader = stub_->AsyncTxn(&context, transaction.txn_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncTxnResponse etcdv3::AsyncDeleteAction::ParseResponse()
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
    txn_resp.prev_values = txn_resp.values;
    txn_resp.action = etcdv3::DELETE_ACTION;
  }
    
  return txn_resp;
}
