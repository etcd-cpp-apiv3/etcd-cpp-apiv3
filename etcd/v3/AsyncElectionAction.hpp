#ifndef __ASYNC_ELECTIONACTION_HPP__
#define __ASYNC_ELECTIONACTION_HPP__

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "proto/v3election.grpc.pb.h"
#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncElectionResponse.hpp"
#include "etcd/Response.hpp"

using grpc::ClientAsyncResponseReader;
using grpc::ClientAsyncReader;
using v3electionpb::CampaignRequest;
using v3electionpb::CampaignResponse;
using v3electionpb::ProclaimRequest;
using v3electionpb::ProclaimResponse;
using v3electionpb::LeaderRequest;
using v3electionpb::LeaderResponse;
using v3electionpb::ResignRequest;
using v3electionpb::ResignResponse;

namespace etcdv3
{
  class AsyncCampaignAction : public etcdv3::Action
  {
    public:
      AsyncCampaignAction(etcdv3::ActionParameters const &param);
      AsyncCampaignResponse ParseResponse();
    private:
      CampaignResponse reply;
      std::unique_ptr<ClientAsyncResponseReader<CampaignResponse>> response_reader;
  };

  class AsyncProclaimAction : public etcdv3::Action
  {
    public:
      AsyncProclaimAction(etcdv3::ActionParameters const &param);
      AsyncProclaimResponse ParseResponse();
    private:
      ProclaimResponse reply;
      std::unique_ptr<ClientAsyncResponseReader<ProclaimResponse>> response_reader;
  };

  class AsyncLeaderAction : public etcdv3::Action
  {
    public:
      AsyncLeaderAction(etcdv3::ActionParameters const &param);
      AsyncLeaderResponse ParseResponse();
    private:
      LeaderResponse reply;
      std::unique_ptr<ClientAsyncResponseReader<LeaderResponse>> response_reader;
  };

  class AsyncObserveAction : public etcdv3::Action
  {
    public:
      AsyncObserveAction(etcdv3::ActionParameters const &param, const bool once=false);
      AsyncObserveResponse ParseResponse();
      void waitForResponse();
      void waitForResponse(std::function<void(etcd::Response)> callback); 
      void CancelObserve();
      bool Cancelled() const;
    private:
      bool once;
      LeaderResponse reply;
      std::unique_ptr<ClientAsyncReader<LeaderResponse>> response_reader;
      std::atomic_bool isCancelled;
      std::mutex protect_is_cancalled;
  };

  class AsyncResignAction : public etcdv3::Action
  {
    public:
      AsyncResignAction(etcdv3::ActionParameters const &param);
      AsyncResignResponse ParseResponse();
    private:
      ResignResponse reply;
      std::unique_ptr<ClientAsyncResponseReader<ResignResponse>> response_reader;
  };
}

#endif
