#include "etcd/Client.hpp"

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
  return send_get(key);
}

pplx::task<etcd::Response> etcd::Client::set(std::string const & key, std::string const & value)
{
  return send_put(key,value);
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


etcd::Response etcd::AsyncPutResponse::ParseResponse()
{
  std::cout << reply.header().revision() << std::endl;
  return etcd::Response();
}

etcd::Response etcd::AsyncRangeResponse::ParseResponse()
{
  mvccpb::KeyValue kvs;
  if(reply.kvs_size())
  {
    int index=0;
    do
    {
      kvs = reply.kvs(index++);
      std::cout<<reply.header().revision() << std::endl;
      std::cout << kvs.create_revision() << std::endl;
      std::cout << kvs.mod_revision() << std::endl;
      std::cout << kvs.version() << std::endl;
    }while(reply.more());
  }
  return etcd::Response();
}

pplx::task<etcd::Response> etcd::Client::send_get(std::string const & key)
{
  RangeRequest request;
  request.set_key(key);
    
  etcd::AsyncRangeResponse* call= new etcd::AsyncRangeResponse();  

  call->response_reader = stub_->AsyncRange(&call->context,request,&call->cq_);

  call->response_reader->Finish(&call->reply, &call->status, (void*)call);
    
  return pplx::task<etcd::Response>([call]()
  {
    void* got_tag;
    bool ok = false;
    etcd::Response resp;

    //blocking
    call->cq_.Next(&got_tag, &ok);
    GPR_ASSERT(got_tag == (void*)call);
    GPR_ASSERT(ok);

    etcd::AsyncRangeResponse* call = static_cast<etcd::AsyncRangeResponse*>(got_tag);
    if(call->status.ok())
    {
      resp = call->ParseResponse();
    }
               
    delete call;
    return resp;
  });
}


pplx::task<etcd::Response> etcd::Client::send_put(std::string const & key, std::string const & value)
{
  PutRequest request;
  request.set_key(key);
  request.set_value(value);
    
  etcd::AsyncPutResponse* call= new etcd::AsyncPutResponse();  

  call->response_reader = stub_->AsyncPut(&call->context,request,&call->cq_);

  call->response_reader->Finish(&call->reply, &call->status, (void*)call);
        

  return pplx::task<etcd::Response>([call]()
  {
    void* got_tag;
    bool ok = false;
    etcd::Response resp;

    //blocking
    call->cq_.Next(&got_tag, &ok);
    GPR_ASSERT(got_tag == (void*)call);
    GPR_ASSERT(ok);

    etcd::AsyncPutResponse* call = static_cast<etcd::AsyncPutResponse*>(got_tag);
    
    if(call->status.ok())
    {
      resp = call->ParseResponse();
    }
    delete call;
    return resp;
  });
}
