#include "etcd/v3/AsyncElectionAction.hpp"

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
    etcdv3::ActionParameters const &param)
  : etcdv3::Action(param)
{
  CampaignRequest campaign_request;
  campaign_request.set_name(param.name);
  campaign_request.set_lease(param.lease_id);
  campaign_request.set_value(param.value);

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
    etcdv3::ActionParameters const &param)
  : etcdv3::Action(param)
{
  auto leader = new LeaderKey();
  leader->set_name(param.name);
  leader->set_key(param.key);
  leader->set_rev(param.revision);
  leader->set_lease(param.lease_id);

  ProclaimRequest proclaim_request;
  proclaim_request.set_allocated_leader(leader);
  proclaim_request.set_value(param.value);

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
    etcdv3::ActionParameters const &param)
  : etcdv3::Action(param)
{
  LeaderRequest leader_request;
  leader_request.set_name(param.name);

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

etcdv3::AsyncObserveAction::AsyncObserveAction(
    etcdv3::ActionParameters const &param, const bool once)
  : etcdv3::Action(param), once(once)
{
  LeaderRequest leader_request;
  leader_request.set_name(param.name);

  response_reader = parameters.election_stub->AsyncObserve(&context, leader_request, &cq_, (void *)etcdv3::ELECTION_OBSERVE_CREATE);

  void *got_tag;
  bool ok = false;
  if (cq_.Next(&got_tag, &ok) && ok && got_tag == (void *)etcdv3::ELECTION_OBSERVE_CREATE) {
    response_reader->Read(&reply, (void *)this);
  } else {
    throw std::runtime_error("failed to create a observe connection");
  }
}

void etcdv3::AsyncObserveAction::waitForResponse() 
{
  void* got_tag;
  bool ok = false;

  while(cq_.Next(&got_tag, &ok))
  {
    if (isCancelled.load()) {
      break;
    }
    if(ok == false)
    {
      break;
    }
    if(got_tag == (void*)this) // read tag
    {
      auto resp = ParseResponse();
      if (resp.get_error_code() != 0) {
        CancelObserve();
        break;
      }
    }
    if(isCancelled.load()) {
      break;
    }
    if (once) {
      break;
    }
    response_reader->Read(&reply, (void *)this);
  }
}

void etcdv3::AsyncObserveAction::waitForResponse(std::function<void(etcd::Response)> callback)
{
  void* got_tag;
  bool ok = false;

  while(cq_.Next(&got_tag, &ok))
  {
    if(ok == false)
    {
      break;
    }
    if (isCancelled.load()) {
      break;
    }
    if(got_tag == (void*)this) // read tag
    {
      auto resp = ParseResponse();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::high_resolution_clock::now() - start_timepoint);
      callback(etcd::Response(resp, duration));
      if (resp.get_error_code() != 0) {
        CancelObserve();
        break;
      }
      start_timepoint = std::chrono::high_resolution_clock::now();
    }
    if(isCancelled.load()) {
      break;
    }
    if (once) {
      break;
    }
    response_reader->Read(&reply, (void *)this);
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
    etcdv3::ActionParameters const &param)
  : etcdv3::Action(param)
{
  auto leader = new LeaderKey();
  leader->set_name(param.name);
  leader->set_key(param.key);
  leader->set_rev(param.revision);
  leader->set_lease(param.lease_id);

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
