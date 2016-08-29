#include "etcd/Client.hpp"
#include "v3/include/AsyncRangeResponse.hpp"
#include "v3/include/AsyncPutResponse.hpp"
#include "v3/include/AsyncTxnResponse.hpp"
#include "v3/include/Utils.hpp"

#include <memory>


using etcdserverpb::TxnRequest;
using etcdserverpb::Compare;
using etcdserverpb::RequestOp;

etcd::Client::Client(std::string const & address)
  : client(address), grpcClient(address)
{
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
  web::http::uri_builder uri("/v2/keys" + key);
  uri.append_query("prevIndex", old_index);
  return send_put_request(uri, "value", value);
}

pplx::task<etcd::Response> etcd::Client::rm(std::string const & key)
{
  web::http::uri_builder uri("/v2/keys" + key);
  uri.append_query("dir=false");
  return Response::create(client.request("DELETE", uri.to_string()));
}

pplx::task<etcd::Response> etcd::Client::rm_if(std::string const & key, std::string const & old_value)
{
  web::http::uri_builder uri("/v2/keys" + key);
  uri.append_query("dir=false");
  uri.append_query("prevValue", old_value);
  return send_del_request(uri);
}

pplx::task<etcd::Response> etcd::Client::rm_if(std::string const & key, int old_index)
{
  web::http::uri_builder uri("/v2/keys" + key);
  uri.append_query("dir=false");
  uri.append_query("prevIndex", old_index);
  return send_del_request(uri);

}

pplx::task<etcd::Response> etcd::Client::mkdir(std::string const & key)
{
  web::http::uri_builder uri("/v2/keys" + key);
  return send_put_request(uri, "dir", "true");
}

pplx::task<etcd::Response> etcd::Client::rmdir(std::string const & key, bool recursive)
{
  web::http::uri_builder uri("/v2/keys" + key);
  uri.append_query("dir=true");
  if (recursive)
    uri.append_query("recursive=true");
  return send_del_request(uri);
}

pplx::task<etcd::Response> etcd::Client::ls(std::string const & key)
{
  web::http::uri_builder uri("/v2/keys" + key);
  uri.append_query("sorted=true");
  return send_get_request(uri);
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





pplx::task<etcd::Response> etcd::Client::send_asyncadd(std::string const & key, std::string const & value)
{

  //check if key is not present
  TxnRequest txn_request;
  Compare* compare = txn_request.add_compare();
  compare->set_result(Compare::CompareResult::Compare_CompareResult_EQUAL);
  compare->set_target(Compare::CompareTarget::Compare_CompareTarget_VERSION);
  compare->set_key(key);
  compare->set_version(0);


  //get key whether success or failure  
  RangeRequest get_request1 = new RangeRequest();
  get_request1->set_key(key);
  RequestOp* req_failure = txn_request.add_failure();
  req_failure->set_allocated_request_range(get_request1);


  //if success, add key and then get new value of key
  PutRequest* put_request = new PutRequest();
  put_request->set_key(key);
  put_request->set_value(value);
  RequestOp* req_success2 = txn_request.add_success();
  req_success2->set_allocated_request_put(put_request);

  RangeRequest* get_request2 = new RangeRequest();
  get_request2->set_key(key);
  RequestOp* req_success3 = txn_request.add_success();
  req_success3->set_allocated_request_range(get_request2);



  etcdv3::AsyncTxnResponse* call= new etcdv3::AsyncTxnResponse("create"); 
   
  call->response_reader = grpcClient.stub_->AsyncTxn(&call->context,txn_request,&call->cq_);

  call->response_reader->Finish(&call->reply, &call->status, (void*)call);

  return Response::create(call);

}

pplx::task<etcd::Response> etcd::Client::send_asyncmodify_if(std::string const & key, std::string const & value, std::string const & old_value)
{

  //check current key is equal to old_value
  etcdv3::AsyncRangeResponse* resp = etcdv3::Utils::getKey(key, grpcClient);
  if(!resp->reply.kvs_size())
  {
    resp->error_code=100;
    resp->error_message="Key not found";
    return Response::createResponse(*resp);
  }
  else
  {
    if(resp->reply.kvs(0).value() != old_value)
    {
      resp->error_code=101;
      resp->error_message="Compare failed";
      return Response::createResponse(*resp);
    }
  }

  PutRequest put_request;
  put_request.set_key(key);
  put_request.set_value(value);
    
  etcdv3::AsyncPutResponse* call= new etcdv3::AsyncPutResponse("compareAndSwap"); 

  //below 2 lines can be removed once we are able to use Txn
  call->prev_value = resp->reply.kvs(0);
  call->client = &grpcClient;
  call->key = key;

  call->response_reader = grpcClient.stub_->AsyncPut(&call->context,put_request,&call->cq_);

  call->response_reader->Finish(&call->reply, &call->status, (void*)call);
        
  return Response::create(call);

}

pplx::task<etcd::Response> etcd::Client::send_asyncmodify(std::string const & key, std::string const & value)
{

  //check if key already exist
  etcdv3::AsyncRangeResponse* resp = etcdv3::Utils::getKey(key, grpcClient);
  if(!resp->reply.kvs_size())
  {
    resp->error_code=100;
    resp->error_message="Key not found";
    return Response::createResponse(*resp);
  }

  PutRequest put_request;
  put_request.set_key(key);
  put_request.set_value(value);
    
  etcdv3::AsyncPutResponse* call= new etcdv3::AsyncPutResponse("update"); 

  //below 2 lines can be removed once we are able to use Txn
  call->prev_value = resp->reply.kvs(0);
  call->client = &grpcClient;
  call->key = key;

  call->response_reader = grpcClient.stub_->AsyncPut(&call->context,put_request,&call->cq_);

  call->response_reader->Finish(&call->reply, &call->status, (void*)call);
        
  return Response::create(call);

}


pplx::task<etcd::Response> etcd::Client::send_asyncget(std::string const & key)
{
  RangeRequest request;
  request.set_key(key);
    
  etcdv3::AsyncRangeResponse* call= new etcdv3::AsyncRangeResponse();  

  call->response_reader = grpcClient.stub_->AsyncRange(&call->context,request,&call->cq_);

  call->response_reader->Finish(&call->reply, &call->status, (void*)call);

  return Response::create(call);
}


pplx::task<etcd::Response> etcd::Client::send_asyncput(std::string const & key, std::string const & value)
{

  PutRequest put_request;
  put_request.set_key(key);
  put_request.set_value(value);
    
  etcdv3::AsyncPutResponse* call= new etcdv3::AsyncPutResponse("set"); 

  //get current value
  etcdv3::AsyncRangeResponse* resp = etcdv3::Utils::getKey(key, grpcClient);
  if(resp->reply.kvs_size())
  {
    call->prev_value = resp->reply.kvs(0);
  }

  call->client = &grpcClient;
  call->key = key;


  call->response_reader = grpcClient.stub_->AsyncPut(&call->context,put_request,&call->cq_);

  call->response_reader->Finish(&call->reply, &call->status, (void*)call);
        
  return Response::create(call);
}


