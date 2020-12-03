#ifndef __ASYNC_LEASETIMETOLIVEACTION_HPP__
#define __ASYNC_LEASETIMETOLIVEACTION_HPP__

#include <grpc++/grpc++.h>

#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncLeaseTimeToLiveResponse.hpp"
#include "proto/rpc.grpc.pb.h"


using etcdserverpb::LeaseTimeToLiveRequest;
using etcdserverpb::LeaseTimeToLiveResponse;
using grpc::ClientAsyncResponseReader;

namespace etcdv3 {
class AsyncLeaseTimeToLiveAction : public etcdv3::Action {
 public:
  AsyncLeaseTimeToLiveAction(etcdv3::ActionParameters param);
  AsyncLeaseTimeToLiveResponse ParseResponse();

 private:
  LeaseTimeToLiveResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<LeaseTimeToLiveResponse>> response_reader;
};
}  // namespace etcdv3

#endif
