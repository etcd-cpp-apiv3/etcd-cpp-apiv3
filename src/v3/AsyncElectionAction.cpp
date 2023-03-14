#include "etcd/v3/AsyncElectionAction.hpp"
#include <grpcpp/support/status.h>

#include "etcd/v3/action_constants.hpp"


using v3electionpb::LeaderKey;
using v3electionpb::CampaignRequest;
using v3electionpb::CampaignResponse;
using v3electionpb::ProclaimRequest;
using v3electionpb::ProclaimResponse;
using v3electionpb::LeaderRequest;
using v3electionpb::LeaderResponse;
using v3electionpb::ResignRequest;
using v3electionpb::ResignResponse;

etcdv3::AsyncCampaignAction::AsyncCampaignAction(
    etcdv3::ActionParameters && params)
  : etcdv3::Action(std::move(params))
{
  CampaignRequest campaign_request;
  campaign_request.set_name(parameters.name);
  campaign_request.set_lease(parameters.lease_id);
  campaign_request.set_value(parameters.value);

  response_reader = parameters.election_stub->AsyncCampaign(&context, campaign_request, &cq_);
  response_reader->Finish(&reply, &status, (void *)this);
}

etcdv3::AsyncCampaignResponse etcdv3::AsyncCampaignAction::ParseResponse()
{
  AsyncCampaignResponse campaign_resp;
  campaign_resp.set_action(etcdv3::CAMPAIGN_ACTION);

  if(!status.ok()) {
    campaign_resp.set_error_code(status.error_code());
    campaign_resp.set_error_message(status.error_message());
  }
  else {
    campaign_resp.ParseResponse(reply);
  }
  return campaign_resp;
}

etcdv3::AsyncProclaimAction::AsyncProclaimAction(
    etcdv3::ActionParameters && params)
  : etcdv3::Action(std::move(params))
{
  auto leader = new LeaderKey();
  leader->set_name(parameters.name);
  leader->set_key(parameters.key);
  leader->set_rev(parameters.revision);
  leader->set_lease(parameters.lease_id);

  ProclaimRequest proclaim_request;
  proclaim_request.set_allocated_leader(leader);
  proclaim_request.set_value(parameters.value);

  response_reader = parameters.election_stub->AsyncProclaim(&context, proclaim_request, &cq_);
  response_reader->Finish(&reply, &status, (void *)this);
}

etcdv3::AsyncProclaimResponse etcdv3::AsyncProclaimAction::ParseResponse()
{
  AsyncProclaimResponse proclaim_resp;
  proclaim_resp.set_action(etcdv3::PROCLAIM_ACTION);
  
  if(!status.ok()) {
    proclaim_resp.set_error_code(status.error_code());
    proclaim_resp.set_error_message(status.error_message());
  }
  else { 
    proclaim_resp.ParseResponse(reply);
  }
  return proclaim_resp;
}

etcdv3::AsyncLeaderAction::AsyncLeaderAction(
    etcdv3::ActionParameters && params)
  : etcdv3::Action(std::move(params))
{
  LeaderRequest leader_request;
  leader_request.set_name(parameters.name);

  response_reader = parameters.election_stub->AsyncLeader(&context, leader_request, &cq_);
  response_reader->Finish(&reply, &status, (void *)this);
}

etcdv3::AsyncLeaderResponse etcdv3::AsyncLeaderAction::ParseResponse()
{
  AsyncLeaderResponse leader_resp;
  leader_resp.set_action(etcdv3::LEADER_ACTION);
  
  if(!status.ok()) {
    leader_resp.set_error_code(status.error_code());
    leader_resp.set_error_message(status.error_message());
  }
  else { 
    leader_resp.ParseResponse(reply);
  }
  return leader_resp;
}

etcdv3::AsyncObserveAction::AsyncObserveAction(etcdv3::ActionParameters && params)
  : etcdv3::Action(std::move(params))
{
  LeaderRequest leader_request;
  leader_request.set_name(parameters.name);

  response_reader = parameters.election_stub->AsyncObserve(&context, leader_request, &cq_, (void *)etcdv3::ELECTION_OBSERVE_CREATE);

  void *got_tag;
  bool ok = false;
  if (cq_.Next(&got_tag, &ok) && ok && got_tag == (void *)etcdv3::ELECTION_OBSERVE_CREATE) {
    // n.b.: leave the issue of `Read` to the `waitForResponse`
  } else {
    throw std::runtime_error("failed to create a observe connection");
  }
}

void etcdv3::AsyncObserveAction::waitForResponse()
{
  void* got_tag;
  bool ok = false;

  if (isCancelled.load()) {
    status = grpc::Status::CANCELLED;
  }
  if (!status.ok()) {
    return;
  }

  response_reader->Read(&reply, (void *)this);
  if (cq_.Next(&got_tag, &ok) && ok && got_tag == (void*)this) {
    auto response = ParseResponse();
    if (response.get_error_code() == 0) {
      // issue the next read
      response_reader->Read(&reply, (void *)this);
    } else {
      this->CancelObserve();
    }
  } else {
    this->CancelObserve();
    status = grpc::Status::CANCELLED;
  }
}

void etcdv3::AsyncObserveAction::CancelObserve()
{
  std::lock_guard<std::mutex> scope_lock(this->protect_is_cancalled);
  if (!isCancelled.exchange(true)) {
    void* got_tag;
    bool ok = false;
    response_reader->Finish(&status, (void *)this);
    if (cq_.Next(&got_tag, &ok) && ok && got_tag == (void *)this) {
      // ok
    } else {
      std::cerr << "Failed to finish a election observing connection" << std::endl;
    }

    cq_.Shutdown();
  }
}

bool etcdv3::AsyncObserveAction::Cancelled() const {
  return isCancelled.load();
}

etcdv3::AsyncObserveResponse etcdv3::AsyncObserveAction::ParseResponse()
{
  AsyncObserveResponse leader_resp;
  leader_resp.set_action(etcdv3::OBSERVE_ACTION);

  if(!status.ok()) {
    leader_resp.set_error_code(status.error_code());
    leader_resp.set_error_message(status.error_message());
  }
  else { 
    leader_resp.ParseResponse(reply);
  }
  return leader_resp;
}

etcdv3::AsyncResignAction::AsyncResignAction(
    etcdv3::ActionParameters && params)
  : etcdv3::Action(std::move(params))
{
  auto leader = new LeaderKey();
  leader->set_name(parameters.name);
  leader->set_key(parameters.key);
  leader->set_rev(parameters.revision);
  leader->set_lease(parameters.lease_id);

  ResignRequest resign_request;
  resign_request.set_allocated_leader(leader);

  response_reader = parameters.election_stub->AsyncResign(&context, resign_request, &cq_);
  response_reader->Finish(&reply, &status, (void *)this);
}

etcdv3::AsyncResignResponse etcdv3::AsyncResignAction::ParseResponse()
{
  AsyncResignResponse resign_resp;
  resign_resp.set_action(etcdv3::RESIGN_ACTION);
  
  if(!status.ok()) {
    resign_resp.set_error_code(status.error_code());
    resign_resp.set_error_message(status.error_message());
  }
  else { 
    resign_resp.ParseResponse(reply);
  }
  return resign_resp;
}
