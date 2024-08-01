#include "etcd/v3/AsyncGRPC.hpp"

#include <cstdlib>

#include <grpcpp/support/status.h>

#include "etcd/Response.hpp"
#include "etcd/v3/Transaction.hpp"
#include "etcd/v3/action_constants.hpp"

using etcdserverpb::DeleteRangeRequest;
using etcdserverpb::LeaseCheckpointRequest;
using etcdserverpb::LeaseGrantRequest;
using etcdserverpb::LeaseKeepAliveRequest;
using etcdserverpb::LeaseLeasesRequest;
using etcdserverpb::LeaseRevokeRequest;
using etcdserverpb::LeaseTimeToLiveRequest;
using etcdserverpb::PutRequest;
using etcdserverpb::RangeRequest;
using etcdserverpb::RequestOp;
using etcdserverpb::ResponseOp;
using etcdserverpb::TxnRequest;
using etcdserverpb::WatchCreateRequest;
using v3electionpb::CampaignRequest;
using v3electionpb::CampaignResponse;
using v3electionpb::LeaderKey;
using v3electionpb::LeaderRequest;
using v3electionpb::LeaderResponse;
using v3electionpb::ProclaimRequest;
using v3electionpb::ProclaimResponse;
using v3electionpb::ResignRequest;
using v3electionpb::ResignResponse;
using v3lockpb::LockRequest;
using v3lockpb::UnlockRequest;

void etcdv3::AsyncCampaignResponse::ParseResponse(CampaignResponse& reply) {
  index = reply.header().revision();

  auto const& leader = reply.leader();
  name = leader.name();
  value.kvs.set_key(leader.key());
  value.kvs.set_create_revision(leader.rev());
  value.kvs.set_lease(leader.lease());
}

void etcdv3::AsyncDeleteResponse::ParseResponse(DeleteRangeResponse& resp) {
  index = resp.header().revision();

  if (resp.prev_kvs_size() == 0) {
    error_code = etcdv3::ERROR_KEY_NOT_FOUND;
    error_message = "etcd-cpp-apiv3: key not found";
  } else {
    // get all previous values
    for (int cnt = 0; cnt < resp.prev_kvs_size(); cnt++) {
      etcdv3::KeyValue kv;
      kv.kvs.CopyFrom(resp.prev_kvs(cnt));
      values.push_back(kv);
      prev_values.push_back(kv);
    }

    // flatten values/prev_values 0 to value/prev_value
    if (!values.empty()) {
      value = values[0];
    }
    if (!prev_values.empty()) {
      prev_value = prev_values[0];
    }
  }
}

void etcdv3::AsyncHeadResponse::ParseResponse(RangeResponse& resp) {
  cluster_id = resp.header().cluster_id();
  member_id = resp.header().member_id();
  index = resp.header().revision();
  raft_term = resp.header().raft_term();
}

void etcdv3::AsyncLeaderResponse::ParseResponse(LeaderResponse& reply) {
  index = reply.header().revision();

  value.kvs = reply.kv();
}

void etcdv3::AsyncLeaseGrantResponse::ParseResponse(LeaseGrantResponse& resp) {
  index = resp.header().revision();
  value.kvs.set_lease(resp.id());
  value.set_ttl(resp.ttl());
  error_message = resp.error();
}

void etcdv3::AsyncLeaseKeepAliveResponse::ParseResponse(
    LeaseKeepAliveResponse& resp) {
  index = resp.header().revision();
  value.kvs.set_lease(resp.id());
  value.set_ttl(resp.ttl());
}

void etcdv3::AsyncMemberAddResponse::ParseResponse(MemberAddResponse& resp) {
  index = resp.header().revision();
  std::string member_type = "Member";
  if (resp.member().islearner()) {
    member_type = "Learner";
  }
  std::cout << "Member (" << resp.member().id() << ")"
            << " Added to the etcd cluster as " << member_type << std::endl;
}

void etcdv3::AsyncMemberListResponse::ParseResponse(MemberListResponse& resp) {
  index = resp.header().revision();
  for (auto member : resp.members()) {
    etcdv3::Member m;
    m.set_id(member.id());
    m.set_name(member.name());

    std::vector<std::string> clientUrlsVec, peerUrlsVec;
    for (const auto& clientUrl : member.clienturls()) {
      clientUrlsVec.push_back(clientUrl);
    }
    for (const auto& peerUrl : member.peerurls()) {
      peerUrlsVec.push_back(peerUrl);
    }

    m.set_clientURLs(clientUrlsVec);
    m.set_peerURLs(peerUrlsVec);

    members.push_back(m);
  }
}

void etcdv3::AsyncMemberRemoveResponse::ParseResponse(
    MemberRemoveResponse& resp) {
  index = resp.header().revision();
}

void etcdv3::AsyncLeaseLeasesResponse::ParseResponse(
    LeaseLeasesResponse& resp) {
  index = resp.header().revision();
  for (auto lease : resp.leases()) {
    leases.emplace_back(lease.id());
  }
}

void etcdv3::AsyncLeaseRevokeResponse::ParseResponse(
    LeaseRevokeResponse& resp) {
  index = resp.header().revision();
}

void etcdv3::AsyncLeaseTimeToLiveResponse::ParseResponse(
    LeaseTimeToLiveResponse& resp) {
  index = resp.header().revision();
  value.kvs.set_lease(resp.id());
  value.set_ttl(resp.ttl());
  // FIXME: unsupported: fields "grantedTTL" and "keys"
}

void etcdv3::AsyncLockResponse::ParseResponse(LockResponse& resp) {
  index = resp.header().revision();
  lock_key = resp.key();
}

void etcdv3::AsyncObserveResponse::ParseResponse(LeaderResponse& reply) {
  index = reply.header().revision();

  value.kvs = reply.kv();
}

void etcdv3::AsyncProclaimResponse::ParseResponse(ProclaimResponse& reply) {
  index = reply.header().revision();
}

void etcdv3::AsyncPutResponse::ParseResponse(PutResponse& resp) {
  index = resp.header().revision();

  // get all previous values
  etcdv3::KeyValue kv;
  kv.kvs.CopyFrom(resp.prev_kv());
  prev_values.push_back(kv);
  prev_value = kv;
}

void etcdv3::AsyncRangeResponse::ParseResponse(RangeResponse& resp,
                                               bool prefix) {
  index = resp.header().revision();
  if (resp.kvs_size() == 0 && !prefix) {
    error_code = etcdv3::ERROR_KEY_NOT_FOUND;
    error_message = "etcd-cpp-apiv3: key not found";
    return;
  } else {
    for (int index = 0; index < resp.kvs_size(); index++) {
      etcdv3::KeyValue kv;
      kv.kvs.CopyFrom(resp.kvs(index));
      values.push_back(kv);
    }

    if (!prefix) {
      value = values[0];
      values.clear();
    }
  }
}

void etcdv3::AsyncResignResponse::ParseResponse(ResignResponse& reply) {
  index = reply.header().revision();
}

void etcdv3::AsyncTxnResponse::ParseResponse(TxnResponse& reply) {
  index = reply.header().revision();
  for (int index = 0; index < reply.responses_size(); index++) {
    auto resp = reply.responses(index);
    if (ResponseOp::ResponseCase::kResponseRange == resp.response_case()) {
      AsyncRangeResponse response;
      response.ParseResponse(*(resp.mutable_response_range()), true);

      if (error_code == 0) {
        error_code = response.get_error_code();
      }
      if (!response.get_error_message().empty()) {
        if (!error_message.empty()) {
          error_message += "\n";
        }
        error_message += response.get_error_message();
      }
      for (auto const& value : response.get_values()) {
        values.emplace_back(value);
      }
      for (auto const& prev_value : response.get_prev_values()) {
        prev_values.emplace_back(prev_value);
      }
    } else if (ResponseOp::ResponseCase::kResponsePut == resp.response_case()) {
      AsyncPutResponse response;
      response.ParseResponse(*(resp.mutable_response_put()));
      if (error_code == 0) {
        error_code = response.get_error_code();
      }
      if (!response.get_error_message().empty()) {
        if (!error_message.empty()) {
          error_message += "\n";
        }
        error_message += response.get_error_message();
      }
      for (auto const& value : response.get_values()) {
        values.emplace_back(value);
      }
      for (auto const& prev_value : response.get_prev_values()) {
        prev_values.emplace_back(prev_value);
      }
    } else if (ResponseOp::ResponseCase::kResponseDeleteRange ==
               resp.response_case()) {
      AsyncDeleteResponse response;
      response.ParseResponse(*(resp.mutable_response_delete_range()));

      // Ignore "key not found" error for delete in txn, keep backwards
      // compatibility.
      if (response.get_error_code() != 0 &&
          response.get_error_code() != etcdv3::ERROR_KEY_NOT_FOUND) {
        error_code = response.get_error_code();
      }
      if (!response.get_error_message().empty()) {
        if (response.get_error_code() != 0 &&
            response.get_error_code() != etcdv3::ERROR_KEY_NOT_FOUND) {
          if (!error_message.empty()) {
            error_message += "\n";
          }
          error_message += response.get_error_message();
        }
      }
      for (auto const& value : response.get_values()) {
        values.emplace_back(value);
      }
      for (auto const& prev_value : response.get_prev_values()) {
        prev_values.emplace_back(prev_value);
      }
    } else if (ResponseOp::ResponseCase::kResponseTxn == resp.response_case()) {
      AsyncTxnResponse response;
      response.ParseResponse(*(resp.mutable_response_txn()));

      if (error_code == 0) {
        error_code = response.get_error_code();
      }
      if (!response.get_error_message().empty()) {
        if (!error_message.empty()) {
          error_message += "\n";
        }
        error_message += response.get_error_message();
      }

      // skip
#ifndef NDEBUG
      std::cerr << "[debug] not implemented error: unable to parse nested "
                   "transaction "
                   "response"
                << std::endl;
#endif
    }
  }
  if (!values.empty()) {
    value = values[0];
  }
  if (!prev_values.empty()) {
    prev_value = prev_values[0];
  }
}

void etcdv3::AsyncUnlockResponse::ParseResponse(UnlockResponse& resp) {
  index = resp.header().revision();
}

void etcdv3::AsyncWatchResponse::ParseResponse(WatchResponse& reply) {
  if (reply.canceled() && reply.compact_revision() != 0) {
    error_code = grpc::StatusCode::OUT_OF_RANGE;
    error_message = "required revision has been compacted";
    compact_revision = reply.compact_revision();
    return;
  }
  index = reply.header().revision();
  for (auto const& e : reply.events()) {
    events.emplace_back(e);
  }
  for (int cnt = 0; cnt < reply.events_size(); cnt++) {
    auto event = reply.events(cnt);
    if (mvccpb::Event::EventType::Event_EventType_PUT == event.type()) {
      if (event.kv().version() == 1) {
        action = etcdv3::CREATE_ACTION;
      } else {
        action = etcdv3::SET_ACTION;
      }
      value.kvs = event.kv();
    } else if (mvccpb::Event::EventType::Event_EventType_DELETE_ ==
               event.type()) {
      action = etcdv3::DELETE_ACTION;
      value.kvs = event.kv();
    }
    if (event.has_prev_kv()) {
      prev_value.kvs = event.prev_kv();
    }
    // just store the first occurence of the key in values.
    // this is done so tas client will not need to change their behaviour.
    // break immediately
    break;
  }
}

etcdv3::AsyncCampaignAction::AsyncCampaignAction(
    etcdv3::ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  CampaignRequest campaign_request;
  campaign_request.set_name(parameters.name);
  campaign_request.set_lease(parameters.lease_id);
  campaign_request.set_value(parameters.value);

  response_reader =
      parameters.election_stub->AsyncCampaign(&context, campaign_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncCampaignResponse etcdv3::AsyncCampaignAction::ParseResponse() {
  AsyncCampaignResponse campaign_resp;
  campaign_resp.set_action(etcdv3::CAMPAIGN_ACTION);

  if (!status.ok()) {
    campaign_resp.set_error_code(status.error_code());
    campaign_resp.set_error_message(status.error_message());
  } else {
    campaign_resp.ParseResponse(reply);
  }
  return campaign_resp;
}

etcdv3::AsyncCompareAndDeleteAction::AsyncCompareAndDeleteAction(
    etcdv3::ActionParameters&& params, etcdv3::AtomicityType type)
    : etcdv3::Action(std::move(params)) {
  etcdv3::Transaction txn;
  if (type == etcdv3::AtomicityType::PREV_VALUE) {
    txn.setup_compare_and_delete(parameters.key, parameters.old_value,
                                 parameters.key);
  } else if (type == etcdv3::AtomicityType::PREV_INDEX) {
    txn.setup_compare_and_delete(parameters.key, parameters.old_revision,
                                 parameters.key);
  }

  response_reader =
      parameters.kv_stub->AsyncTxn(&context, *txn.txn_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncTxnResponse etcdv3::AsyncCompareAndDeleteAction::ParseResponse() {
  AsyncTxnResponse txn_resp;
  txn_resp.set_action(etcdv3::COMPAREDELETE_ACTION);

  if (!status.ok()) {
    txn_resp.set_error_code(status.error_code());
    txn_resp.set_error_message(status.error_message());
  } else {
    txn_resp.ParseResponse(reply);

    if (!reply.succeeded()) {
      txn_resp.set_error_code(ERROR_COMPARE_FAILED);
      txn_resp.set_error_message("etcd-cpp-apiv3: compare failed");
    }
  }
  return txn_resp;
}

etcdv3::AsyncCompareAndSwapAction::AsyncCompareAndSwapAction(
    etcdv3::ActionParameters&& params, etcdv3::AtomicityType type)
    : etcdv3::Action(std::move(params)) {
  etcdv3::Transaction txn;
  if (type == etcdv3::AtomicityType::PREV_VALUE) {
    txn.setup_compare_and_swap(parameters.key, parameters.old_value,
                               parameters.value, parameters.lease_id);
  } else if (type == etcdv3::AtomicityType::PREV_INDEX) {
    txn.setup_compare_and_swap(parameters.key, parameters.old_revision,
                               parameters.value, parameters.lease_id);
  }
  // backwards compatibility
  txn.add_success_range(parameters.key);

  response_reader =
      parameters.kv_stub->AsyncTxn(&context, *txn.txn_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncTxnResponse etcdv3::AsyncCompareAndSwapAction::ParseResponse() {
  AsyncTxnResponse txn_resp;
  txn_resp.set_action(etcdv3::COMPARESWAP_ACTION);

  if (!status.ok()) {
    txn_resp.set_error_code(status.error_code());
    txn_resp.set_error_message(status.error_message());
  } else {
    txn_resp.ParseResponse(reply);

    // if there is an error code returned by parseResponse, we must
    // not overwrite it.
    if (!reply.succeeded() && !txn_resp.get_error_code()) {
      txn_resp.set_error_code(ERROR_COMPARE_FAILED);
      txn_resp.set_error_message("etcd-cpp-apiv3: compare failed");
    }
  }
  return txn_resp;
}

etcdv3::AsyncDeleteAction::AsyncDeleteAction(ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  DeleteRangeRequest del_request;
  detail::make_request_with_ranges(del_request, parameters.key,
                                   parameters.range_end, parameters.withPrefix);
  del_request.set_prev_kv(true /* fetch prev values */);

  response_reader =
      parameters.kv_stub->AsyncDeleteRange(&context, del_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncDeleteResponse etcdv3::AsyncDeleteAction::ParseResponse() {
  AsyncDeleteResponse del_resp;
  del_resp.set_action(etcdv3::DELETE_ACTION);

  if (!status.ok()) {
    del_resp.set_error_code(status.error_code());
    del_resp.set_error_message(status.error_message());
  } else {
    del_resp.ParseResponse(reply);
  }
  return del_resp;
}

etcdv3::AsyncHeadAction::AsyncHeadAction(etcdv3::ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  RangeRequest get_request;
  get_request.set_key(etcdv3::NUL);
  get_request.set_limit(1);
  response_reader = parameters.kv_stub->AsyncRange(&context, get_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncHeadResponse etcdv3::AsyncHeadAction::ParseResponse() {
  AsyncHeadResponse head_resp;
  head_resp.set_action(etcdv3::GET_ACTION);

  if (!status.ok()) {
    head_resp.set_error_code(status.error_code());
    head_resp.set_error_message(status.error_message());
  } else {
    head_resp.ParseResponse(reply);
  }
  return head_resp;
}

etcdv3::AsyncLeaderAction::AsyncLeaderAction(etcdv3::ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  LeaderRequest leader_request;
  leader_request.set_name(parameters.name);

  response_reader =
      parameters.election_stub->AsyncLeader(&context, leader_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncLeaderResponse etcdv3::AsyncLeaderAction::ParseResponse() {
  AsyncLeaderResponse leader_resp;
  leader_resp.set_action(etcdv3::LEADER_ACTION);

  if (!status.ok()) {
    leader_resp.set_error_code(status.error_code());
    leader_resp.set_error_message(status.error_message());
  } else {
    leader_resp.ParseResponse(reply);
  }
  return leader_resp;
}

etcdv3::AsyncLeaseGrantAction::AsyncLeaseGrantAction(
    etcdv3::ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  LeaseGrantRequest leasegrant_request;
  leasegrant_request.set_ttl(parameters.ttl);
  // If ID is set to 0, etcd will choose an ID.
  leasegrant_request.set_id(parameters.lease_id);

  response_reader = parameters.lease_stub->AsyncLeaseGrant(
      &context, leasegrant_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncLeaseGrantResponse etcdv3::AsyncLeaseGrantAction::ParseResponse() {
  AsyncLeaseGrantResponse lease_resp;
  lease_resp.set_action(etcdv3::LEASEGRANT);

  if (!status.ok()) {
    lease_resp.set_error_code(status.error_code());
    lease_resp.set_error_message(status.error_message());
  } else {
    lease_resp.ParseResponse(reply);
  }
  return lease_resp;
}

etcdv3::AsyncLeaseKeepAliveAction::AsyncLeaseKeepAliveAction(
    etcdv3::ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  isCancelled = false;
  stream = parameters.lease_stub->AsyncLeaseKeepAlive(
      &context, &cq_, (void*) etcdv3::KEEPALIVE_CREATE);

  void* got_tag = nullptr;
  bool ok = false;
  if (cq_.Next(&got_tag, &ok) && ok &&
      got_tag == (void*) etcdv3::KEEPALIVE_CREATE) {
    // ok
  } else {
    status = grpc::Status(grpc::StatusCode::CANCELLED,
                          "Failed to create a lease keep-alive connection");
    // cannot continue for further refresh
    isCancelled.store(true);
  }
}

etcdv3::AsyncLeaseKeepAliveResponse
etcdv3::AsyncLeaseKeepAliveAction::ParseResponse() {
  AsyncLeaseKeepAliveResponse lease_resp;
  lease_resp.set_action(etcdv3::LEASEKEEPALIVE);

  if (!status.ok()) {
    lease_resp.set_error_code(status.error_code());
    lease_resp.set_error_message(status.error_message());
  } else {
    lease_resp.ParseResponse(reply);
  }
  return lease_resp;
}

etcd::Response etcdv3::AsyncLeaseKeepAliveAction::Refresh() {
  std::lock_guard<std::recursive_mutex> scope_lock(this->protect_is_cancelled);

  auto start_timepoint = std::chrono::high_resolution_clock::now();
  if (isCancelled.load()) {
    status = grpc::Status::CANCELLED;
    return etcd::Response(ParseResponse(),
                          etcd::detail::duration_till_now(start_timepoint));
  }

  LeaseKeepAliveRequest leasekeepalive_request;
  leasekeepalive_request.set_id(parameters.lease_id);

  void* got_tag = nullptr;
  bool ok = false;

  if (parameters.has_grpc_timeout()) {
    stream->Write(leasekeepalive_request, (void*) etcdv3::KEEPALIVE_WRITE);
    // wait write finish
    switch (cq_.AsyncNext(&got_tag, &ok, parameters.grpc_deadline())) {
    case CompletionQueue::NextStatus::TIMEOUT: {
      status = grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED,
                            "gRPC timeout during keep alive write");
      break;
    }
    case CompletionQueue::NextStatus::SHUTDOWN: {
      status = grpc::Status(grpc::StatusCode::UNAVAILABLE,
                            "gRPC already shutdown during keep alive write");
      break;
    }
    case CompletionQueue::NextStatus::GOT_EVENT: {
      if (!ok || got_tag != (void*) etcdv3::KEEPALIVE_WRITE) {
        return etcd::Response(grpc::StatusCode::ABORTED,
                              "Failed to create a lease keep-alive connection: "
                              "write not ok or invalid tag");
      }
    }
    }
    if (!status.ok()) {
      this->CancelKeepAlive();
      return etcd::Response(ParseResponse(),
                            etcd::detail::duration_till_now(start_timepoint));
    }

    stream->Read(&reply, (void*) etcdv3::KEEPALIVE_READ);
    // wait read finish
    switch (cq_.AsyncNext(&got_tag, &ok, parameters.grpc_deadline())) {
    case CompletionQueue::NextStatus::TIMEOUT: {
      status = grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED,
                            "gRPC timeout during keep alive read");
      break;
    }
    case CompletionQueue::NextStatus::SHUTDOWN: {
      status = grpc::Status(grpc::StatusCode::UNAVAILABLE,
                            "gRPC already shutdown during keep alive read");
      break;
    }
    case CompletionQueue::NextStatus::GOT_EVENT: {
      if (ok && got_tag == (void*) etcdv3::KEEPALIVE_READ) {
        return etcd::Response(ParseResponse(),
                              etcd::detail::duration_till_now(start_timepoint));
      }
      break;
    }
    }
    this->CancelKeepAlive();
    return etcd::Response(grpc::StatusCode::ABORTED,
                          "Failed to create a lease keep-alive connection: "
                          "read not ok or invalid tag");
  } else {
    stream->Write(leasekeepalive_request, (void*) etcdv3::KEEPALIVE_WRITE);
    // wait write finish
    if (cq_.Next(&got_tag, &ok) && ok &&
        got_tag == (void*) etcdv3::KEEPALIVE_WRITE) {
      stream->Read(&reply, (void*) etcdv3::KEEPALIVE_READ);
      // wait read finish
      if (cq_.Next(&got_tag, &ok) && ok &&
          got_tag == (void*) etcdv3::KEEPALIVE_READ) {
        return etcd::Response(ParseResponse(),
                              etcd::detail::duration_till_now(start_timepoint));
      }
    }
    this->CancelKeepAlive();
    return etcd::Response(grpc::StatusCode::ABORTED,
                          "Failed to create a lease keep-alive connection: "
                          "read not ok or invalid tag");
  }
}

void etcdv3::AsyncLeaseKeepAliveAction::CancelKeepAlive() {
  std::lock_guard<std::recursive_mutex> scope_lock(this->protect_is_cancelled);
  if (!isCancelled.exchange(true)) {
    void* got_tag = nullptr;
    bool ok = false;

    stream->WritesDone((void*) etcdv3::KEEPALIVE_DONE);
    if (cq_.Next(&got_tag, &ok) && ok &&
        got_tag == (void*) etcdv3::KEEPALIVE_DONE) {
      // ok
    } else {
#ifndef NDEBUG
      std::cerr
          << "[debug] failed to mark a lease keep-alive connection as DONE: "
          << context.debug_error_string() << std::endl;
#endif
    }

    stream->Finish(&status, (void*) KEEPALIVE_FINISH);
    if (cq_.Next(&got_tag, &ok) && ok && got_tag == (void*) KEEPALIVE_FINISH) {
      // ok
    } else {
#ifndef NDEBUG
      std::cerr << "[debug] failed to finish a lease keep-alive connection: "
                << status.error_message() << ", "
                << context.debug_error_string() << std::endl;
#endif
    }

    // cancel on-the-fly calls
    context.TryCancel();

    cq_.Shutdown();
  }
}

bool etcdv3::AsyncLeaseKeepAliveAction::Cancelled() const {
  return isCancelled.load();
}

etcdv3::ActionParameters&
etcdv3::AsyncLeaseKeepAliveAction::mutable_parameters() {
  return this->parameters;
}

etcdv3::AsyncAddMemberAction::AsyncAddMemberAction(
    etcdv3::ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  MemberAddRequest add_member_request;

  for (const auto& url : parameters.peer_urls) {
    add_member_request.add_peerurls(url);
  }
  add_member_request.set_islearner(parameters.is_learner);
  response_reader = parameters.cluster_stub->AsyncMemberAdd(
      &context, add_member_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncMemberAddResponse etcdv3::AsyncAddMemberAction::ParseResponse() {
  AsyncMemberAddResponse add_member_resp;
  add_member_resp.set_action(etcdv3::ADDMEMBER);

  if (!status.ok()) {
    add_member_resp.set_error_code(status.error_code());
    add_member_resp.set_error_message(status.error_message());
  } else {
    add_member_resp.ParseResponse(reply);
  }
  return add_member_resp;
}

etcdv3::AsyncListMemberAction::AsyncListMemberAction(
    etcdv3::ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  MemberListRequest member_list_request;

  response_reader = parameters.cluster_stub->AsyncMemberList(
      &context, member_list_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncMemberListResponse etcdv3::AsyncListMemberAction::ParseResponse() {
  AsyncMemberListResponse list_member_resp;
  list_member_resp.set_action(etcdv3::LISTMEMBER);

  if (!status.ok()) {
    list_member_resp.set_error_code(status.error_code());
    list_member_resp.set_error_message(status.error_message());
  } else {
    list_member_resp.ParseResponse(reply);
  }
  return list_member_resp;
}

etcdv3::AsyncRemoveMemberAction::AsyncRemoveMemberAction(
    etcdv3::ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  MemberRemoveRequest remove_member_request;

  remove_member_request.set_id(parameters.member_id);
  response_reader = parameters.cluster_stub->AsyncMemberRemove(
      &context, remove_member_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncMemberRemoveResponse
etcdv3::AsyncRemoveMemberAction::ParseResponse() {
  AsyncMemberRemoveResponse remove_member_resp;
  remove_member_resp.set_action(etcdv3::REMOVEMEMBER);

  if (!status.ok()) {
    remove_member_resp.set_error_code(status.error_code());
    remove_member_resp.set_error_message(status.error_message());
  } else {
    remove_member_resp.ParseResponse(reply);
  }
  return remove_member_resp;
}

etcdv3::AsyncLeaseLeasesAction::AsyncLeaseLeasesAction(
    etcdv3::ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  LeaseLeasesRequest leaseleases_request;

  response_reader = parameters.lease_stub->AsyncLeaseLeases(
      &context, leaseleases_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncLeaseLeasesResponse
etcdv3::AsyncLeaseLeasesAction::ParseResponse() {
  AsyncLeaseLeasesResponse lease_resp;
  lease_resp.set_action(etcdv3::LEASELEASES);

  if (!status.ok()) {
    lease_resp.set_error_code(status.error_code());
    lease_resp.set_error_message(status.error_message());
  } else {
    lease_resp.ParseResponse(reply);
  }
  return lease_resp;
}

etcdv3::AsyncLeaseRevokeAction::AsyncLeaseRevokeAction(
    etcdv3::ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  LeaseRevokeRequest leaserevoke_request;
  leaserevoke_request.set_id(parameters.lease_id);

  response_reader = parameters.lease_stub->AsyncLeaseRevoke(
      &context, leaserevoke_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncLeaseRevokeResponse
etcdv3::AsyncLeaseRevokeAction::ParseResponse() {
  AsyncLeaseRevokeResponse lease_resp;
  lease_resp.set_action(etcdv3::LEASEREVOKE);

  if (!status.ok()) {
    lease_resp.set_error_code(status.error_code());
    lease_resp.set_error_message(status.error_message());
  } else {
    lease_resp.ParseResponse(reply);
  }
  return lease_resp;
}

etcdv3::AsyncLeaseTimeToLiveAction::AsyncLeaseTimeToLiveAction(
    etcdv3::ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  LeaseTimeToLiveRequest leasetimetolive_request;
  leasetimetolive_request.set_id(parameters.lease_id);
  // FIXME: unsupported parameters: "keys"
  // leasetimetolive_request.set_keys(parameters.keys);

  response_reader = parameters.lease_stub->AsyncLeaseTimeToLive(
      &context, leasetimetolive_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncLeaseTimeToLiveResponse
etcdv3::AsyncLeaseTimeToLiveAction::ParseResponse() {
  AsyncLeaseTimeToLiveResponse lease_resp;
  lease_resp.set_action(etcdv3::LEASETIMETOLIVE);

  if (!status.ok()) {
    lease_resp.set_error_code(status.error_code());
    lease_resp.set_error_message(status.error_message());
  } else {
    lease_resp.ParseResponse(reply);
  }
  return lease_resp;
}

etcdv3::AsyncLockAction::AsyncLockAction(ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  LockRequest lock_request;
  lock_request.set_name(parameters.key);
  lock_request.set_lease(parameters.lease_id);

  response_reader =
      parameters.lock_stub->AsyncLock(&context, lock_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncLockResponse etcdv3::AsyncLockAction::ParseResponse() {
  AsyncLockResponse lock_resp;
  lock_resp.set_action(etcdv3::LOCK_ACTION);

  if (!status.ok()) {
    lock_resp.set_error_code(status.error_code());
    lock_resp.set_error_message(status.error_message());
  } else {
    lock_resp.ParseResponse(reply);
  }

  return lock_resp;
}

etcdv3::AsyncObserveAction::AsyncObserveAction(
    etcdv3::ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  LeaderRequest leader_request;
  leader_request.set_name(parameters.name);

  response_reader = parameters.election_stub->AsyncObserve(
      &context, leader_request, &cq_, (void*) etcdv3::ELECTION_OBSERVE_CREATE);

  void* got_tag;
  bool ok = false;
  if (cq_.Next(&got_tag, &ok) && ok &&
      got_tag == (void*) etcdv3::ELECTION_OBSERVE_CREATE) {
    // n.b.: leave the issue of `Read` to the `waitForResponse`
  } else {
    status = grpc::Status(grpc::StatusCode::CANCELLED,
                          "failed to create a observe connection");
    // cannot continue for further observing
    isCancelled.store(true);
  }
}

void etcdv3::AsyncObserveAction::waitForResponse() {
  void* got_tag;
  bool ok = false;

  if (isCancelled.load()) {
    status = grpc::Status::CANCELLED;
  }
  if (!status.ok()) {
    return;
  }

  response_reader->Read(&reply, (void*) this);
  if (cq_.Next(&got_tag, &ok) && ok && got_tag == (void*) this) {
    auto response = ParseResponse();
    if (response.get_error_code() != 0) {
      this->CancelObserve();
    }
  } else {
    this->CancelObserve();
    status = grpc::Status::CANCELLED;
  }
}

void etcdv3::AsyncObserveAction::CancelObserve() {
  std::lock_guard<std::mutex> scope_lock(this->protect_is_cancelled);
  if (!isCancelled.exchange(true)) {
    void* got_tag;
    bool ok = false;

    response_reader->Finish(&status, (void*) ELECTION_OBSERVE_FINISH);

    // FIXME: not sure why the `Next()` after `Finish()` blocks forever.
    // Using the `AsyncNext()` without a timeout to ensure the cancel is done.
    switch (cq_.AsyncNext(
        &got_tag, &ok,
        std::chrono::system_clock::now() + std::chrono::microseconds(1))) {
    case CompletionQueue::NextStatus::TIMEOUT:
    case CompletionQueue::NextStatus::SHUTDOWN:
      // ignore
      break;
    case CompletionQueue::NextStatus::GOT_EVENT:
      if (!ok || got_tag != (void*) ELECTION_OBSERVE_FINISH) {
#ifndef NDEBUG
        std::cerr << "[debug] failed to finish a election observing connection"
                  << std::endl;
#endif
      }
    }

    // cancel on-the-fly calls
    context.TryCancel();

    cq_.Shutdown();
  }
}

bool etcdv3::AsyncObserveAction::Cancelled() const {
  return isCancelled.load();
}

etcdv3::AsyncObserveResponse etcdv3::AsyncObserveAction::ParseResponse() {
  AsyncObserveResponse leader_resp;
  leader_resp.set_action(etcdv3::OBSERVE_ACTION);

  if (!status.ok()) {
    leader_resp.set_error_code(status.error_code());
    leader_resp.set_error_message(status.error_message());
  } else {
    leader_resp.ParseResponse(reply);
  }
  return leader_resp;
}

etcdv3::AsyncProclaimAction::AsyncProclaimAction(
    etcdv3::ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  auto leader = new LeaderKey();
  leader->set_name(parameters.name);
  leader->set_key(parameters.key);
  leader->set_rev(parameters.revision);
  leader->set_lease(parameters.lease_id);

  ProclaimRequest proclaim_request;
  proclaim_request.set_allocated_leader(leader);
  proclaim_request.set_value(parameters.value);

  response_reader =
      parameters.election_stub->AsyncProclaim(&context, proclaim_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncProclaimResponse etcdv3::AsyncProclaimAction::ParseResponse() {
  AsyncProclaimResponse proclaim_resp;
  proclaim_resp.set_action(etcdv3::PROCLAIM_ACTION);

  if (!status.ok()) {
    proclaim_resp.set_error_code(status.error_code());
    proclaim_resp.set_error_message(status.error_message());
  } else {
    proclaim_resp.ParseResponse(reply);
  }
  return proclaim_resp;
}

etcdv3::AsyncPutAction::AsyncPutAction(ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  PutRequest put_request;
  put_request.set_key(parameters.key);
  put_request.set_value(parameters.value);
  put_request.set_lease(parameters.lease_id);
  put_request.set_prev_kv(true);

  response_reader = parameters.kv_stub->AsyncPut(&context, put_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncPutResponse etcdv3::AsyncPutAction::ParseResponse() {
  AsyncPutResponse put_resp;
  put_resp.set_action(etcdv3::PUT_ACTION);

  if (!status.ok()) {
    put_resp.set_error_code(status.error_code());
    put_resp.set_error_message(status.error_message());
  } else {
    put_resp.ParseResponse(reply);
  }

  return put_resp;
}

etcdv3::AsyncRangeAction::AsyncRangeAction(etcdv3::ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  RangeRequest get_request;
  detail::make_request_with_ranges(get_request, parameters.key,
                                   parameters.range_end, parameters.withPrefix);
  if (parameters.revision > 0) {
    get_request.set_revision(parameters.revision);
  }

  get_request.set_limit(parameters.limit);
  get_request.set_sort_order(
      RangeRequest::SortOrder::RangeRequest_SortOrder_NONE);

  // set keys_only and count_only
  get_request.set_keys_only(params.keys_only);
  get_request.set_count_only(params.count_only);

  response_reader = parameters.kv_stub->AsyncRange(&context, get_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncRangeResponse etcdv3::AsyncRangeAction::ParseResponse() {
  AsyncRangeResponse range_resp;
  range_resp.set_action(etcdv3::GET_ACTION);

  if (!status.ok()) {
    range_resp.set_error_code(status.error_code());
    range_resp.set_error_message(status.error_message());
  } else {
    range_resp.ParseResponse(
        reply, parameters.withPrefix || !parameters.range_end.empty());
  }
  return range_resp;
}

etcdv3::AsyncResignAction::AsyncResignAction(etcdv3::ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  auto leader = new LeaderKey();
  leader->set_name(parameters.name);
  leader->set_key(parameters.key);
  leader->set_rev(parameters.revision);
  leader->set_lease(parameters.lease_id);

  ResignRequest resign_request;
  resign_request.set_allocated_leader(leader);

  response_reader =
      parameters.election_stub->AsyncResign(&context, resign_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncResignResponse etcdv3::AsyncResignAction::ParseResponse() {
  AsyncResignResponse resign_resp;
  resign_resp.set_action(etcdv3::RESIGN_ACTION);

  if (!status.ok()) {
    resign_resp.set_error_code(status.error_code());
    resign_resp.set_error_message(status.error_message());
  } else {
    resign_resp.ParseResponse(reply);
  }
  return resign_resp;
}

etcdv3::AsyncSetAction::AsyncSetAction(etcdv3::ActionParameters&& params,
                                       bool create)
    : etcdv3::Action(std::move(params)) {
  etcdv3::Transaction txn;
  isCreate = create;
  txn.add_compare_mod(parameters.key, 0 /* not exists */);
  txn.add_success_put(parameters.key, parameters.value, parameters.lease_id);
  // backwards compatibility
  txn.add_success_range(parameters.key);
  if (create) {
    txn.add_failure_range(parameters.key);
  } else {
    txn.add_failure_put(parameters.key, parameters.value, parameters.lease_id);
  }
  response_reader =
      parameters.kv_stub->AsyncTxn(&context, *txn.txn_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncTxnResponse etcdv3::AsyncSetAction::ParseResponse() {
  AsyncTxnResponse txn_resp;
  txn_resp.set_action(isCreate ? etcdv3::CREATE_ACTION : etcdv3::SET_ACTION);

  if (!status.ok()) {
    txn_resp.set_error_code(status.error_code());
    txn_resp.set_error_message(status.error_message());
  } else {
    txn_resp.ParseResponse(reply);

    if (!reply.succeeded() && isCreate) {
      txn_resp.set_error_code(etcdv3::ERROR_KEY_ALREADY_EXISTS);
      txn_resp.set_error_message("etcd-cpp-apiv3: key already exists");
    }
  }
  return txn_resp;
}

etcdv3::AsyncTxnAction::AsyncTxnAction(etcdv3::ActionParameters&& params,
                                       etcdv3::Transaction const& tx)
    : etcdv3::Action(std::move(params)) {
  response_reader =
      parameters.kv_stub->AsyncTxn(&context, *tx.txn_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncTxnResponse etcdv3::AsyncTxnAction::ParseResponse() {
  AsyncTxnResponse txn_resp;
  txn_resp.set_action(etcdv3::TXN_ACTION);

  if (!status.ok()) {
    txn_resp.set_error_code(status.error_code());
    txn_resp.set_error_message(status.error_message());
  } else {
    txn_resp.ParseResponse(reply);

    // if there is an error code returned by parseResponse, we must
    // not overwrite it.
    if (!reply.succeeded() && !txn_resp.get_error_code()) {
      txn_resp.set_error_code(ERROR_COMPARE_FAILED);
      txn_resp.set_error_message("etcd-cpp-apiv3: compare failed");
    }
  }

  return txn_resp;
}

etcdv3::AsyncUnlockAction::AsyncUnlockAction(ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  UnlockRequest unlock_request;
  unlock_request.set_key(parameters.key);

  response_reader =
      parameters.lock_stub->AsyncUnlock(&context, unlock_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncUnlockResponse etcdv3::AsyncUnlockAction::ParseResponse() {
  AsyncUnlockResponse unlock_resp;
  unlock_resp.set_action(etcdv3::UNLOCK_ACTION);

  if (!status.ok()) {
    unlock_resp.set_error_code(status.error_code());
    unlock_resp.set_error_message(status.error_message());
  } else {
    unlock_resp.ParseResponse(reply);
  }

  return unlock_resp;
}

etcdv3::AsyncUpdateAction::AsyncUpdateAction(etcdv3::ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  etcdv3::Transaction txn;
  txn.add_compare_version(parameters.key, CompareResult::GREATER, 0);  // exists
  txn.add_success_put(parameters.key, parameters.value, parameters.lease_id,
                      true /* for backwards compatibility */);
  // backwards compatibility
  txn.add_success_range(parameters.key);
  txn.add_failure_range(parameters.key);
  response_reader =
      parameters.kv_stub->AsyncTxn(&context, *txn.txn_request, &cq_);
  response_reader->Finish(&reply, &status, (void*) this);
}

etcdv3::AsyncTxnResponse etcdv3::AsyncUpdateAction::ParseResponse() {
  AsyncTxnResponse txn_resp;

  if (!status.ok()) {
    txn_resp.set_error_code(status.error_code());
    txn_resp.set_error_message(status.error_message());
  } else {
    if (reply.succeeded()) {
      txn_resp.ParseResponse(reply);
      txn_resp.set_action(etcdv3::UPDATE_ACTION);
    } else {
      txn_resp.set_error_code(etcdv3::ERROR_KEY_NOT_FOUND);
      txn_resp.set_error_message("etcd-cpp-apiv3: key not found");
    }
  }
  return txn_resp;
}

etcdv3::AsyncWatchAction::AsyncWatchAction(etcdv3::ActionParameters&& params)
    : etcdv3::Action(std::move(params)) {
  isCancelled.store(false);
  stream = parameters.watch_stub->AsyncWatch(&context, &cq_,
                                             (void*) etcdv3::WATCH_CREATE);
  this->watch_id =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  // #ifndef NDEBUG
  //   std::clog << "etcd-cpp-apiv3: watch_id: " << this->watch_id << std::endl;
  // #endif

  WatchRequest watch_req;
  WatchCreateRequest watch_create_req;
  detail::make_request_with_ranges(watch_create_req, parameters.key,
                                   parameters.range_end, parameters.withPrefix);
  watch_create_req.set_prev_kv(true);
  watch_create_req.set_start_revision(parameters.revision);
  watch_create_req.set_watch_id(this->watch_id);

  watch_req.mutable_create_request()->CopyFrom(watch_create_req);

  // wait "create" success (the stream becomes ready)
  void* got_tag;
  bool ok = false;
  if (cq_.Next(&got_tag, &ok) && ok &&
      got_tag == (void*) etcdv3::WATCH_CREATE) {
    stream->Write(watch_req, (void*) etcdv3::WATCH_WRITE);
  } else {
    status = grpc::Status(grpc::StatusCode::CANCELLED,
                          "failed to create a watch connection");
    // cannot continue for further watching
    isCancelled.store(true);
  }

  if (!status.ok()) {
    return;
  }

  // wait "write" (WatchCreateRequest) success, and start to read the first
  // reply
  if (cq_.Next(&got_tag, &ok) && ok && got_tag == (void*) etcdv3::WATCH_WRITE) {
    stream->Read(&reply, (void*) this);
  } else {
    status = grpc::Status(grpc::StatusCode::CANCELLED,
                          "failed to write WatchCreateRequest to server");
    // cannot continue for further watching
    isCancelled.store(true);
  }
}

/**
 * Notes: `Cancel` and `waitForResponse` of watchers.
 *
 * We meet failures about failed to cancel the watcher on Ubuntu 20.04
 * due to unable to receive the "etcdv3::WATCH_FINISH" tag from the gRPC
 * completion queue.
 *
 * See CI:
 * https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3/actions/runs/5561458372/jobs/10159155857
 *
 * To address the problem, we use the `AsyncNext()` to wait for the
 * the last token from the completion queue (wait for 1 second, once
 * we called the method `stream->Finish()`).
 *
 * Remark: the issue might be caused by lower version etcd.
 */

void etcdv3::AsyncWatchAction::waitForResponse() {
  void* got_tag;
  bool ok = false;
  bool the_final_round = false;

  // failed to create the watcher
  if (!status.ok()) {
    return;
  }

  while (true) {
    if (!the_final_round) {
      if (!cq_.Next(&got_tag, &ok)) {
        break;
      }
    } else {
      auto deadline =
          std::chrono::system_clock::now() + std::chrono::seconds(1);
      switch (cq_.AsyncNext(&got_tag, &ok, deadline)) {
      case CompletionQueue::NextStatus::TIMEOUT:
      case CompletionQueue::NextStatus::SHUTDOWN: {
#ifndef NDEBUG
        std::cerr << "[warn] watcher does't exit normally" << std::endl;
#endif
        // pretend to be received a "WATCH_FINISH" tag: shutdown
        context.TryCancel();
        cq_.Shutdown();
        ok = false;  // jump out
        break;
      }
      case CompletionQueue::NextStatus::GOT_EVENT: {
        // normal execution flow
        break;
      }
      }
    }
    if (ok == false) {
      break;
    }
    if (got_tag == (void*) etcdv3::WATCH_WRITE_CANCEL) {
      stream->WritesDone((void*) etcdv3::WATCH_WRITES_DONE);
      continue;
    }
    if (got_tag == (void*) etcdv3::WATCH_WRITES_DONE) {
      the_final_round = true;
      stream->Finish(&status, (void*) etcdv3::WATCH_FINISH);
      continue;
    }
    if (got_tag == (void*) etcdv3::WATCH_FINISH) {
      // shutdown
      cq_.Shutdown();
      break;
    }
    if (got_tag == (void*) this)  // read tag
    {
      if (reply.canceled()) {
        // cancel on-the-fly calls, but don't shutdown the completion queue as
        // there are still a inflight call to finish
        context.TryCancel();
        continue;
      }

      // we stop watch under two conditions:
      //
      // 1. watch for a future revision, return immediately with empty events
      //    set
      // 2. receive any effective events.
      if ((reply.created() &&
           reply.header().revision() < parameters.revision) ||
          reply.events_size() > 0) {
        // leave a warning if the response is too large and been fragmented
        if (reply.fragment()) {
          std::cerr
              << "WARN: The response hasn't been fully received and parsed"
              << std::endl;
        }

        // cancel the watcher after receiving the good response
        this->CancelWatch();

        // start the next round to read finish messages, read into "&dummy"
        // (use nullptr, as it won't be touched).
        stream->Read(nullptr, (void*) etcdv3::WATCH_FINISH);
      } else {
        // start the next round to read reply, read into "&reply"
        stream->Read(&reply, (void*) this);
      }
      continue;
    }
    if (isCancelled.load()) {
      // invalid tag, and is cancelled
      break;
    }
  }
}

void etcdv3::AsyncWatchAction::waitForResponse(
    std::function<void(etcd::Response)> callback) {
  void* got_tag;
  bool ok = false;
  bool the_final_round = false;

  // failed to create the watcher
  if (!status.ok()) {
    auto resp = ParseResponse();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - start_timepoint);
    callback(etcd::Response(resp, duration));
  }

  while (true) {
    if (!the_final_round) {
      if (!cq_.Next(&got_tag, &ok)) {
        break;
      }
    } else {
      auto deadline =
          std::chrono::system_clock::now() + std::chrono::seconds(1);
      switch (cq_.AsyncNext(&got_tag, &ok, deadline)) {
      case CompletionQueue::NextStatus::TIMEOUT:
      case CompletionQueue::NextStatus::SHUTDOWN: {
        std::cerr << "[warn] watcher does't exit normally" << std::endl;
        // pretend to be received a "WATCH_FINISH" tag: shutdown
        context.TryCancel();
        cq_.Shutdown();
        ok = false;  // jump out
        break;
      }
      case CompletionQueue::NextStatus::GOT_EVENT: {
        // normal execution flow
        break;
      }
      }
    }
    if (ok == false) {
      break;
    }
    if (got_tag == (void*) etcdv3::WATCH_WRITE_CANCEL) {
      stream->WritesDone((void*) etcdv3::WATCH_WRITES_DONE);
      continue;
    }
    if (got_tag == (void*) etcdv3::WATCH_WRITES_DONE) {
      the_final_round = true;
      stream->Finish(&status, (void*) etcdv3::WATCH_FINISH);
      continue;
    }
    if (got_tag == (void*) etcdv3::WATCH_FINISH) {
      // shutdown
      cq_.Shutdown();
      break;
    }
    if (got_tag == (void*) this)  // read tag
    {
      if (reply.canceled()) {
        if (reply.compact_revision() != 0) {
          auto resp = ParseResponse();
          auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::high_resolution_clock::now() - start_timepoint);
          callback(etcd::Response(resp, duration));
        }
        // cancel on-the-fly calls, but don't shutdown the completion queue as
        // there are still a inflight call to finish
        context.TryCancel();
        continue;
      }

      // for the callback case, we don't invoke callback immediately if watching
      // for a future revision, we wait until there are some effective events.
      if (reply.events_size()) {
        auto resp = ParseResponse();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - start_timepoint);
        callback(etcd::Response(resp, duration));
        start_timepoint = std::chrono::high_resolution_clock::now();
      }
      stream->Read(&reply, (void*) this);
      continue;
    }
    if (isCancelled.load()) {
      // invalid tag, and is cancelled
      break;
    }
  }
}

void etcdv3::AsyncWatchAction::CancelWatch() {
  if (!isCancelled.exchange(true)) {
    WatchRequest cancel_req;
    cancel_req.mutable_cancel_request()->set_watch_id(this->watch_id);
    stream->Write(cancel_req, (void*) etcdv3::WATCH_WRITE_CANCEL);
    isCancelled.store(true);
  }
}

bool etcdv3::AsyncWatchAction::Cancelled() const { return isCancelled.load(); }

etcdv3::AsyncWatchResponse etcdv3::AsyncWatchAction::ParseResponse() {
  AsyncWatchResponse watch_resp;
  watch_resp.set_action(etcdv3::WATCH_ACTION);
  watch_resp.set_watch_id(reply.watch_id());

  if (!status.ok()) {
    watch_resp.set_error_code(status.error_code());
    watch_resp.set_error_message(status.error_message());
  } else {
    watch_resp.ParseResponse(reply);
  }
  return watch_resp;
}
