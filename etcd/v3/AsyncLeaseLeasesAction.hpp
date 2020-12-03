#ifndef __ASYNC_LEASELEASESACTION_HPP__
#define __ASYNC_LEASELEASESACTION_HPP__

#include <grpc++/grpc++.h>

#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncLeaseLeasesResponse.hpp"
#include "proto/rpc.grpc.pb.h"

using etcdserverpb::LeaseLeasesRequest;
using etcdserverpb::LeaseLeasesResponse;
using grpc::ClientAsyncResponseReader;

namespace etcdv3 {
class AsyncLeaseLeasesAction : public etcdv3::Action {
 public:
  AsyncLeaseLeasesAction(etcdv3::ActionParameters param);
  AsyncLeaseLeasesResponse ParseResponse();

 private:
  LeaseLeasesResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<LeaseLeasesResponse>> response_reader;
};
}  // namespace etcdv3

#endif
