#ifndef __ASYNC_LEASEGRANTACTION_HPP__
#define __ASYNC_LEASEGRANTACTION_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncLeaseGrantResponse.hpp"

using grpc::ClientAsyncResponseReader;
using etcdserverpb::LeaseGrantResponse;

namespace etcdv3
{
  class AsyncLeaseGrantAction : public etcdv3::Action
  {
    public:
      AsyncLeaseGrantAction(etcdv3::ActionParameters param);
      AsyncLeaseGrantResponse ParseResponse();
    private:
      LeaseGrantResponse reply;
      std::unique_ptr<ClientAsyncResponseReader<LeaseGrantResponse>> response_reader;
  };
}

#endif
