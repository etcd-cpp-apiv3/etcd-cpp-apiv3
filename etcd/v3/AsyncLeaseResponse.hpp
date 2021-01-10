#ifndef __ASYNC_LEASERESPONSE_HPP__
#define __ASYNC_LEASERESPONSE_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "etcd/v3/V3Response.hpp"

using etcdserverpb::LeaseGrantResponse;
using etcdserverpb::LeaseRevokeResponse;
using etcdserverpb::LeaseCheckpoint;
using etcdserverpb::LeaseCheckpointResponse;
using etcdserverpb::LeaseKeepAliveResponse;
using etcdserverpb::LeaseTimeToLiveResponse;
using etcdserverpb::LeaseStatus;
using etcdserverpb::LeaseLeasesResponse;

namespace etcdv3
{
  class AsyncLeaseGrantResponse : public etcdv3::V3Response
  {
    public:
      AsyncLeaseGrantResponse(){};
      void ParseResponse(LeaseGrantResponse& resp);
  };

  class AsyncLeaseRevokeResponse : public etcdv3::V3Response
  {
    public:
      AsyncLeaseRevokeResponse(){};
      void ParseResponse(LeaseRevokeResponse& resp);
  };

  class AsyncLeaseKeepAliveResponse : public etcdv3::V3Response
  {
    public:
      AsyncLeaseKeepAliveResponse(){};
      void ParseResponse(LeaseKeepAliveResponse& resp);
  };

  class AsyncLeaseTimeToLiveResponse : public etcdv3::V3Response
  {
    public:
      AsyncLeaseTimeToLiveResponse(){};
      void ParseResponse(LeaseTimeToLiveResponse& resp);
  };

  class AsyncLeaseLeasesResponse : public etcdv3::V3Response
  {
    public:
      AsyncLeaseLeasesResponse(){};
      void ParseResponse(LeaseLeasesResponse& resp);
  };
}

#endif
