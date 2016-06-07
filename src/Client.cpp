#include "etcd/Client.hpp"

#include <iostream>
#include "etcd/DeleteRpcResponse.h"

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
    watchServiceStub = Watch::NewStub(channel);
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
  web::http::uri_builder uri("/v2/keys" + key);
  return send_get_request(uri);
}

pplx::task<etcd::Response> etcd::Client::set(std::string const & key, std::string const & value)
{
  web::http::uri_builder uri("/v2/keys" + key);
  return send_put_request(uri, "value", value);
}

void etcd::Client::setv3(std::string const &key, std::string const &value)
{
	std::cout << "FBDL setv3" << std::endl;
	etcdserverpb::PutRequest putRequest;
	putRequest.set_key(key);
	putRequest.set_value(value);

	etcdserverpb::PutResponse putResponse;
	grpc::ClientContext context;
	grpc::Status status = stub_->Put(&context, putRequest, &putResponse);

	if(status.ok()){
		std::cout << "put OK" << std::endl;
	}
	else {
		std::cout << "put NOK" << std::endl;
	}
}

pplx::task<etcd::Response> etcd::Client::add(std::string const & key, std::string const & value)
{
  web::http::uri_builder uri("/v2/keys" + key);
  uri.append_query("prevExist=false");
  return send_put_request(uri, "value", value);
}

pplx::task<etcd::Response> etcd::Client::modify(std::string const & key, std::string const & value)
{
  web::http::uri_builder uri("/v2/keys" + key);
  uri.append_query("prevExist=true");
  return send_put_request(uri, "value", value);
}

pplx::task<etcd::Response> etcd::Client::modify_if(std::string const & key, std::string const & value, std::string const & old_value)
{
  web::http::uri_builder uri("/v2/keys" + key);
  uri.append_query("prevValue", old_value);
  return send_put_request(uri, "value", value);
}

//FBDL
pplx::task<etcd::Response> etcd::Client::modify_if(std::string const & key, std::string const & value, int old_index)
{
  web::http::uri_builder uri("/v2/keys" + key);
  uri.append_query("prevIndex", old_index);
  return send_put_request(uri, "value", value);
}

pplx::task<etcd::Response> etcd::Client::removeEntry(std::string const & entryKey) {

	etcd::DeleteRpcResponse *drp = new etcd::DeleteRpcResponse();

	//get muna
	std::cout<<"blocking call for get rpc first " << entryKey << std::endl;
	etcdserverpb::RangeRequest rangeRequest;
	rangeRequest.set_key(entryKey);

	etcdserverpb::RangeResponse rangeResponse;
	grpc::ClientContext context;

	grpc::Status status = stub_->Range(&context, rangeRequest, &rangeResponse);

	if(status.ok()) {
		std::cout << "get OK" << std::endl;
		drp->fillUpV2ResponseValues(rangeResponse);
	}
	else {
		std::cout << "get NOK" << std::endl;
	}


	//then delete
	std::cout << "removing etcd v3 entry naman with template na" << std::endl;

	etcdserverpb::DeleteRangeRequest deleteRangeRequest;
	deleteRangeRequest.set_key(entryKey);

	drp->rpcInstance = stub_->AsyncDeleteRange(&drp->context, deleteRangeRequest, &drp->cq_);
	drp->rpcInstance->Finish(&drp->deleteResponse, &drp->status, (void*)drp);

	return Response::create(drp);

//	return pplx::task<etcd::Response>([drp]()
//		{
//			std::cout << "doing delete v3 entry task" << std::endl;
//			void* got_tag;
//			bool ok = false;
//			etcd::Response resp;
//
//			drp->completionQueue.Next(&got_tag, &ok);
//			GPR_ASSERT(got_tag == (void*)drp);
//			GPR_ASSERT(ok);
//
//			etcd::DeleteRpcResponse* deleteResponse = static_cast<etcd::DeleteRpcResponse*>(got_tag);
//
//			if (deleteResponse->status.ok()){
//				std::cout << "doing delete v3 entry task OK" << std::endl;
//				resp = *drp; //simply stripping off the response part
//			}
//			else
//				std::cout << "doing delete v3 entry task NOK" << std::endl;
//			delete deleteResponse;
//			std::cout << "delete done, returning" << std::endl;
//			return resp;
//		}
//	);
}



void etcd::Client::getv3(std::string const & key) {
	std::cout<<"blocking call for get rpc " << key << std::endl;
	etcdserverpb::RangeRequest rangeRequest;
	rangeRequest.set_key(key);

	etcdserverpb::RangeResponse rangeResponse;
	grpc::ClientContext context;

	grpc::Status status = stub_->Range(&context, rangeRequest, &rangeResponse);

	std::cout << "checking status" << std::endl;
	if(status.ok()) {
		std::cout << "get OK" << std::endl;
		std::cout << "size: " << rangeResponse.kvs_size() << std::endl;
		std::cout << "kvs 0 key: " << rangeResponse.kvs(0).key() << std::endl;
		std::cout << "kvs 0 value: " << rangeResponse.kvs(0).value() << std::endl;
		std::cout << "kvs.Get 0 value: " << rangeResponse.kvs().Get(0).value() << std::endl;

		DeleteRpcResponse drp;
		drp.fillUpV2ResponseValues(rangeResponse);
	}
	else {
		std::cout << "get NOK" << std::endl;
	}
}

pplx::task<etcd::Response> etcd::Client::rm(std::string const & key)
{
	return removeEntry(key);
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


