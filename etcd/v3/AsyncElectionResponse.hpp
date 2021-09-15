#ifndef __ASYNC_ELECTIONRESPONSE_HPP__
#define __ASYNC_ELECTIONRESPONSE_HPP__

#include "proto/rpc.pb.h"
#include "proto/v3election.pb.h"
#include "etcd/v3/V3Response.hpp"

using v3electionpb::CampaignResponse;
using v3electionpb::ProclaimResponse;
using v3electionpb::LeaderResponse;
using v3electionpb::ResignResponse;

namespace etcdv3
{
  class AsyncCampaignResponse : public etcdv3::V3Response
  {
    public:
      AsyncCampaignResponse(){};
      void ParseResponse(CampaignResponse& resp);
  };

  class AsyncProclaimResponse : public etcdv3::V3Response
  {
    public:
      AsyncProclaimResponse(){};
      void ParseResponse(ProclaimResponse& resp);
  };

  class AsyncLeaderResponse : public etcdv3::V3Response
  {
    public:
      AsyncLeaderResponse(){};
      void ParseResponse(LeaderResponse& resp);
  };

  class AsyncObserveResponse : public etcdv3::V3Response
  {
    public:
      AsyncObserveResponse(){};
      void ParseResponse(LeaderResponse& resp);
  };

  class AsyncResignResponse : public etcdv3::V3Response
  {
    public:
      AsyncResignResponse(){};
      void ParseResponse(ResignResponse& resp);
  };
}

#endif
