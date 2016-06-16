#ifndef __GRPC_CLIENT_HPP__
#define __GRPC_CLIENT_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "v3/include/AsyncRangeResponse.hpp"


using grpc::Channel;
using etcdserverpb::PutRequest;
using etcdserverpb::RangeRequest;
using etcdserverpb::KV;

namespace etcdv3
{

  class grpcClient
  {
  public:
    grpcClient(std::string const & address);
    std::unique_ptr<KV::Stub> stub_;
  };
}

#endif
