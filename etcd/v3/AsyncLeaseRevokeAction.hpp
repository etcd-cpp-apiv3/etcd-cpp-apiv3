#ifndef __ASYNC_LEASEREVOKEACTION_HPP__
#define __ASYNC_LEASEREVOKEACTION_HPP__

#include <grpc++/grpc++.h>

#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncLeaseRevokeResponse.hpp"
#include "proto/rpc.grpc.pb.h"


using etcdserverpb::LeaseRevokeRequest;
using etcdserverpb::LeaseRevokeResponse;
using grpc::ClientAsyncResponseReader;

namespace etcdv3 {
class AsyncLeaseRevokeAction : public etcdv3::Action {
 public:
  AsyncLeaseRevokeAction(etcdv3::ActionParameters param);
  AsyncLeaseRevokeResponse ParseResponse();

 private:
  LeaseRevokeResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<LeaseRevokeResponse>> response_reader;
};
}  // namespace etcdv3

#endif
