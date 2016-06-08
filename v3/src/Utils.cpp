#include "v3/include/AsyncRangeResponse.hpp"
#include "proto/rpc.grpc.pb.h"
using etcdserverpb::RangeRequest;

#include "v3/include/Utils.hpp"


etcdv3::AsyncRangeResponse* etcdv3::Utils::getKey(std::string const & key, etcdv3::grpcClient& client)
{
  RangeRequest get_request;
  get_request.set_key(key);
  etcdv3::AsyncRangeResponse* resp= new etcdv3::AsyncRangeResponse();   

  resp->status = client.stub_->Range(&resp->context, get_request, &resp->reply);
  
  if(resp->status.ok())
  {
    return resp;
  }
  else
  {
    throw std::runtime_error(resp->status.error_message());
  }
}
