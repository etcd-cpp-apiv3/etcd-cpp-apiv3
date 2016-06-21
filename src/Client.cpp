#include <memory>
#include "etcd/Client.hpp"
#include "v3/include/AsyncTxnResponse.hpp"
#include "v3/include/AsyncRangeResponse.hpp"
#include "v3/include/Utils.hpp"
#include "v3/include/Transaction.hpp"
#include <iostream>

using grpc::Channel;
using etcdserverpb::PutRequest;
using etcdserverpb::RangeRequest;
using etcdserverpb::TxnRequest;
using etcdserverpb::DeleteRangeRequest;
using etcdserverpb::Compare;
using etcdserverpb::RequestOp;

using grpc::ClientReaderWriter;
using etcdserverpb::WatchRequest;
using etcdserverpb::WatchResponse;
using etcdserverpb::WatchCreateRequest;

etcd::Client::Client(std::string const & address)
  : client(address)
{
  std::string stripped_address(address);
  std::string substr("http://");
  std::string::size_type i = stripped_address.find(substr);
  if(i != std::string::npos)
  {
    stripped_address.erase(i,substr.length());
  }
  std::shared_ptr<Channel> channel = grpc::CreateChannel(stripped_address, grpc::InsecureChannelCredentials());
  stub_= KV::NewStub(channel);
}

pplx::task<etcd::Response> etcd::Client::send_get_request(web::http::uri_builder & uri)
{
  return Response::create(client.request(web::http::methods::GET, uri.to_string()));
}

pplx::task<etcd::Response> etcd::Client::send_del_request(web::http::uri_builder & uri)
{
  return Response::create(client.request(web::http::methods::DEL, uri.to_string()));
}

pplx::task<etcd::Response> etcd::Client::send_put_request(web::http::uri_builder & uri, std::string const & key, std::string const & value)
{
  std::string data = key + "=" + value;
  std::string content_type = "application/x-www-form-urlencoded; param=" + key;
  return Response::create(client.request(web::http::methods::PUT, uri.to_string(), data.c_str(), content_type.c_str()));
}

pplx::task<etcd::Response> etcd::Client::get(std::string const & key)
{
  return send_asyncget(key);
}

pplx::task<etcd::Response> etcd::Client::set(std::string const & key, std::string const & value)
{
  return send_asyncput(key,value);
}

pplx::task<etcd::Response> etcd::Client::add(std::string const & key, std::string const & value)
{
  return send_asyncadd(key,value);
}

pplx::task<etcd::Response> etcd::Client::modify(std::string const & key, std::string const & value)
{
  return send_asyncmodify(key,value);
}


pplx::task<etcd::Response> etcd::Client::modify_if(std::string const & key, std::string const & value, std::string const & old_value)
{
  return send_asyncmodify_if(key, value, old_value);
}


pplx::task<etcd::Response> etcd::Client::modify_if(std::string const & key, std::string const & value, int old_index)
{
	return send_asyncmodify_if(key, value, old_index);
}


pplx::task<etcd::Response> etcd::Client::rm(std::string const & key)
{
	return send_asyncdelete(key,false);
}


pplx::task<etcd::Response> etcd::Client::rm_if(std::string const & key, std::string const & old_value)
{
	return send_asyncrm_if(key, old_value);
}


pplx::task<etcd::Response> etcd::Client::rm_if(std::string const & key, int old_index)
{
	return send_asyncrm_if(key, old_index);

}


pplx::task<etcd::Response> etcd::Client::mkdir(std::string const & key)
{
  web::http::uri_builder uri("/v2/keys" + key);
  return send_put_request(uri, "dir", "true");
}


pplx::task<etcd::Response> etcd::Client::rmdir(std::string const & key, bool recursive)
{

  return send_asyncdelete(key,recursive);
}


pplx::task<etcd::Response> etcd::Client::ls(std::string const & key)
{

  std::string range_end(key); 
  int ascii = (int)range_end[range_end.length()-1];
  range_end.back() = ascii+1;

  return send_asyncget(key,range_end);
}


pplx::task<etcd::Response> etcd::Client::watch(std::string const & key, bool recursive)
{
  web::http::uri_builder uri("/v2/keys" + key);
  uri.append_query("wait=true");
  if (recursive)
    uri.append_query("recursive=true");
  return send_get_request(uri);
}


pplx::task<etcd::Response> etcd::Client::watch(std::string const & key, int fromIndex, bool recursive)
{
  web::http::uri_builder uri("/v2/keys" + key);
  uri.append_query("wait=true");
  uri.append_query("waitIndex", fromIndex);
  if (recursive)
    uri.append_query("recursive=true");
  return send_get_request(uri);
}


std::shared_ptr<etcdv3::AsyncTxnResponse> etcd::Client::initiate_transaction(const std::string &operation,
		etcdv3::Transaction& transaction)
{
	std::shared_ptr<etcdv3::AsyncTxnResponse> call(new etcdv3::AsyncTxnResponse(operation));
	call->response_reader = stub_->AsyncTxn(&call->context, transaction.txn_request, &call->cq_);
	call->response_reader->Finish(&call->reply, &call->status, (void*) (call.get()));
	return call;
}


pplx::task<etcd::Response> etcd::Client::send_asyncadd(std::string const & key, std::string const & value)
{
  etcdv3::Transaction transaction(key);
  transaction.init_compare(Compare::CompareResult::Compare_CompareResult_EQUAL,
		  	  	  	  	  	  Compare::CompareTarget::Compare_CompareTarget_VERSION);

  transaction.setup_basic_failure_operation(key);
  transaction.setup_basic_create_sequence(key, value);

  std::shared_ptr<etcdv3::AsyncTxnResponse> call = initiate_transaction("create", transaction);
  return Response::create(call);
}


pplx::task<etcd::Response> etcd::Client::send_asyncmodify_if(std::string const & key, std::string const & value, std::string const & old_value)
{
  etcdv3::Transaction transaction(key);
  transaction.init_compare(old_value, Compare::CompareResult::Compare_CompareResult_EQUAL,
		  	  	  	  	  	  	  	  Compare::CompareTarget::Compare_CompareTarget_VALUE);

  transaction.setup_basic_failure_operation(key);
  transaction.setup_compare_and_swap_sequence(value);

  std::shared_ptr<etcdv3::AsyncTxnResponse> call = initiate_transaction("compareAndSwap", transaction);
  return Response::create(call);
}


pplx::task<etcd::Response> etcd::Client::send_asyncmodify_if(std::string const & key, std::string const & value, int old_index)
{
  etcdv3::Transaction transaction(key);
  transaction.init_compare(old_index, Compare::CompareResult::Compare_CompareResult_EQUAL,
		  	  	  	  	  	  	  	  Compare::CompareTarget::Compare_CompareTarget_MOD);

  transaction.setup_basic_failure_operation(key);
  transaction.setup_compare_and_swap_sequence(value);

  std::shared_ptr<etcdv3::AsyncTxnResponse> call = initiate_transaction("compareAndSwap", transaction);
  return Response::create(call);
}


pplx::task<etcd::Response> etcd::Client::send_asyncmodify(std::string const & key, std::string const & value)
{
  etcdv3::Transaction transaction(key);
  transaction.init_compare(Compare::CompareResult::Compare_CompareResult_GREATER,
		  	  	  	  	  	  Compare::CompareTarget::Compare_CompareTarget_VERSION);

  transaction.setup_basic_failure_operation(key);
  transaction.setup_compare_and_swap_sequence(value);

  std::shared_ptr<etcdv3::AsyncTxnResponse> call = initiate_transaction("update", transaction);
  return Response::create(call);
}


pplx::task<etcd::Response> etcd::Client::send_asyncget(std::string const & key, std::string const& range_end)
{
  RangeRequest get_request;
  get_request.set_key(key);
  if(!range_end.empty())
  {
    get_request.set_range_end(range_end);
    get_request.set_sort_target(RangeRequest::SortTarget::RangeRequest_SortTarget_KEY);
    get_request.set_sort_order(RangeRequest::SortOrder::RangeRequest_SortOrder_ASCEND);
  }
    
  std::shared_ptr<etcdv3::AsyncRangeResponse> call(new etcdv3::AsyncRangeResponse());  
  call->response_reader = stub_->AsyncRange(&call->context,get_request,&call->cq_);
  call->response_reader->Finish(&call->reply, &call->status, (void*)call.get());

  return Response::create(call);
}


pplx::task<etcd::Response> etcd::Client::send_asyncput(std::string const & key, std::string const & value)
{
  etcdv3::Transaction transaction(key);
  transaction.init_compare(Compare::CompareResult::Compare_CompareResult_EQUAL,
		  	  	  	  	  	  Compare::CompareTarget::Compare_CompareTarget_VERSION);

  transaction.setup_set_failure_operation(key, value);
  transaction.setup_basic_create_sequence(key, value);
        
  std::shared_ptr<etcdv3::AsyncTxnResponse> call = initiate_transaction("set", transaction);
  return Response::create(call);
}


pplx::task<etcd::Response> etcd::Client::send_asyncdelete(std::string const & key, bool recursive)
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

  std::shared_ptr<etcdv3::AsyncTxnResponse> call = initiate_transaction("delete", transaction);
  return Response::create(call);
}


pplx::task<etcd::Response> etcd::Client::send_asyncrm_if(std::string const &key, std::string const &old_value) 
{
  etcdv3::Transaction transaction(key);
  transaction.init_compare(old_value, Compare::CompareResult::Compare_CompareResult_EQUAL,
		  Compare::CompareTarget::Compare_CompareTarget_VALUE);

  transaction.setup_compare_and_delete_operation(key);
  transaction.setup_basic_failure_operation(key);

  std::shared_ptr<etcdv3::AsyncTxnResponse> call = initiate_transaction("compareAndDelete", transaction);
  return Response::create(call);
}


pplx::task<etcd::Response> etcd::Client::send_asyncrm_if(std::string const &key, int old_index) {
	etcdv3::Transaction transaction(key);
	transaction.init_compare(old_index, Compare::CompareResult::Compare_CompareResult_EQUAL,
								Compare::CompareTarget::Compare_CompareTarget_MOD);

	transaction.setup_compare_and_delete_operation(key);
	transaction.setup_basic_failure_operation(key);

	std::shared_ptr<etcdv3::AsyncTxnResponse> call = initiate_transaction("compareAndDelete", transaction);
	return Response::create(call);
}
