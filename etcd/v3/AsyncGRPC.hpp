#ifndef __ASYNC_GRPC_HPP__
#define __ASYNC_GRPC_HPP__

#include <atomic>
#include <mutex>

#include <grpc++/grpc++.h>

#include "proto/rpc.grpc.pb.h"
#include "proto/rpc.pb.h"
#include "proto/v3election.grpc.pb.h"
#include "proto/v3election.pb.h"
#include "proto/v3lock.grpc.pb.h"
#include "proto/v3lock.pb.h"

#include "etcd/Response.hpp"
#include "etcd/v3/Action.hpp"
#include "etcd/v3/V3Response.hpp"

using grpc::ClientAsyncReader;
using grpc::ClientAsyncReaderWriter;
using grpc::ClientAsyncResponseReader;

using etcdserverpb::KV;

using etcdserverpb::DeleteRangeRequest;
using etcdserverpb::DeleteRangeResponse;
using etcdserverpb::LeaseCheckpointRequest;
using etcdserverpb::LeaseCheckpointResponse;
using etcdserverpb::LeaseGrantRequest;
using etcdserverpb::LeaseGrantResponse;
using etcdserverpb::LeaseKeepAliveRequest;
using etcdserverpb::LeaseKeepAliveResponse;
using etcdserverpb::LeaseLeasesRequest;
using etcdserverpb::LeaseLeasesResponse;
using etcdserverpb::LeaseRevokeRequest;
using etcdserverpb::LeaseRevokeResponse;
using etcdserverpb::LeaseTimeToLiveRequest;
using etcdserverpb::LeaseTimeToLiveResponse;
using etcdserverpb::MemberAddRequest;
using etcdserverpb::MemberAddResponse;
using etcdserverpb::MemberListRequest;
using etcdserverpb::MemberListResponse;
using etcdserverpb::MemberRemoveRequest;
using etcdserverpb::MemberRemoveResponse;
using etcdserverpb::PutRequest;
using etcdserverpb::PutResponse;
using etcdserverpb::RangeRequest;
using etcdserverpb::RangeResponse;
using etcdserverpb::TxnRequest;
using etcdserverpb::TxnResponse;
using etcdserverpb::WatchRequest;
using etcdserverpb::WatchResponse;
using v3electionpb::CampaignRequest;
using v3electionpb::CampaignResponse;
using v3electionpb::LeaderRequest;
using v3electionpb::LeaderResponse;
using v3electionpb::ProclaimRequest;
using v3electionpb::ProclaimResponse;
using v3electionpb::ResignRequest;
using v3electionpb::ResignResponse;
using v3lockpb::LockRequest;
using v3lockpb::LockResponse;
using v3lockpb::UnlockRequest;
using v3lockpb::UnlockResponse;

namespace etcd {
class KeepAlive;
}

namespace etcdv3 {
class Transaction;
}

namespace etcdv3 {
class AsyncCampaignResponse : public etcdv3::V3Response {
 public:
  AsyncCampaignResponse(){};
  void ParseResponse(CampaignResponse& resp);
};

class AsyncDeleteResponse : public etcdv3::V3Response {
 public:
  AsyncDeleteResponse(){};
  void ParseResponse(DeleteRangeResponse& resp);
};

class AsyncHeadResponse : public etcdv3::V3Response {
 public:
  AsyncHeadResponse(){};
  void ParseResponse(RangeResponse& resp);
};

class AsyncLeaderResponse : public etcdv3::V3Response {
 public:
  AsyncLeaderResponse(){};
  void ParseResponse(LeaderResponse& resp);
};

class AsyncLeaseGrantResponse : public etcdv3::V3Response {
 public:
  AsyncLeaseGrantResponse(){};
  void ParseResponse(LeaseGrantResponse& resp);
};

class AsyncLeaseKeepAliveResponse : public etcdv3::V3Response {
 public:
  AsyncLeaseKeepAliveResponse(){};
  void ParseResponse(LeaseKeepAliveResponse& resp);
};

class AsyncMemberAddResponse : public etcdv3::V3Response {
 public:
  AsyncMemberAddResponse(){};
  void ParseResponse(MemberAddResponse& resp);
};

class AsyncMemberListResponse : public etcdv3::V3Response {
 public:
  AsyncMemberListResponse(){};
  void ParseResponse(MemberListResponse& resp);
};

class AsyncMemberRemoveResponse : public etcdv3::V3Response {
 public:
  AsyncMemberRemoveResponse(){};
  void ParseResponse(MemberRemoveResponse& resp);
};

class AsyncLeaseLeasesResponse : public etcdv3::V3Response {
 public:
  AsyncLeaseLeasesResponse(){};
  void ParseResponse(LeaseLeasesResponse& resp);
};

class AsyncLeaseRevokeResponse : public etcdv3::V3Response {
 public:
  AsyncLeaseRevokeResponse(){};
  void ParseResponse(LeaseRevokeResponse& resp);
};

class AsyncLeaseTimeToLiveResponse : public etcdv3::V3Response {
 public:
  AsyncLeaseTimeToLiveResponse(){};
  void ParseResponse(LeaseTimeToLiveResponse& resp);
};

class AsyncLockResponse : public etcdv3::V3Response {
 public:
  AsyncLockResponse(){};
  void ParseResponse(LockResponse& resp);
};

class AsyncObserveResponse : public etcdv3::V3Response {
 public:
  AsyncObserveResponse(){};
  void ParseResponse(LeaderResponse& resp);
};

class AsyncProclaimResponse : public etcdv3::V3Response {
 public:
  AsyncProclaimResponse(){};
  void ParseResponse(ProclaimResponse& resp);
};

class AsyncPutResponse : public etcdv3::V3Response {
 public:
  AsyncPutResponse(){};
  void ParseResponse(PutResponse& resp);
};

class AsyncRangeResponse : public etcdv3::V3Response {
 public:
  AsyncRangeResponse(){};
  void ParseResponse(RangeResponse& resp, bool prefix = false);
};

class AsyncResignResponse : public etcdv3::V3Response {
 public:
  AsyncResignResponse(){};
  void ParseResponse(ResignResponse& resp);
};

class AsyncTxnResponse : public etcdv3::V3Response {
 public:
  AsyncTxnResponse(){};
  void ParseResponse(TxnResponse& resp);
};

class AsyncUnlockResponse : public etcdv3::V3Response {
 public:
  AsyncUnlockResponse(){};
  void ParseResponse(UnlockResponse& resp);
};

class AsyncWatchResponse : public etcdv3::V3Response {
 public:
  AsyncWatchResponse(){};
  void ParseResponse(WatchResponse& resp);
};
}  // namespace etcdv3

namespace etcdv3 {
class AsyncCampaignAction : public etcdv3::Action {
 public:
  AsyncCampaignAction(etcdv3::ActionParameters&& params);
  AsyncCampaignResponse ParseResponse();

 private:
  CampaignResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<CampaignResponse>> response_reader;
};

class AsyncCompareAndDeleteAction : public etcdv3::Action {
 public:
  AsyncCompareAndDeleteAction(etcdv3::ActionParameters&& params,
                              etcdv3::AtomicityType type);
  AsyncTxnResponse ParseResponse();

 private:
  TxnResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<TxnResponse>> response_reader;
};

class AsyncCompareAndSwapAction : public etcdv3::Action {
 public:
  AsyncCompareAndSwapAction(etcdv3::ActionParameters&& params,
                            etcdv3::AtomicityType type);
  AsyncTxnResponse ParseResponse();

 private:
  TxnResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<TxnResponse>> response_reader;
};

class AsyncDeleteAction : public etcdv3::Action {
 public:
  AsyncDeleteAction(etcdv3::ActionParameters&& params);
  AsyncDeleteResponse ParseResponse();

 private:
  DeleteRangeResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<DeleteRangeResponse>>
      response_reader;
};

class AsyncHeadAction : public etcdv3::Action {
 public:
  AsyncHeadAction(etcdv3::ActionParameters&& params);
  AsyncHeadResponse ParseResponse();

 private:
  RangeResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<RangeResponse>> response_reader;
};

class AsyncLeaderAction : public etcdv3::Action {
 public:
  AsyncLeaderAction(etcdv3::ActionParameters&& params);
  AsyncLeaderResponse ParseResponse();

 private:
  LeaderResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<LeaderResponse>> response_reader;
};

class AsyncLeaseGrantAction : public etcdv3::Action {
 public:
  AsyncLeaseGrantAction(etcdv3::ActionParameters&& params);
  AsyncLeaseGrantResponse ParseResponse();

 private:
  LeaseGrantResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<LeaseGrantResponse>>
      response_reader;
};

class AsyncLeaseKeepAliveAction : public etcdv3::Action {
 public:
  AsyncLeaseKeepAliveAction(etcdv3::ActionParameters&& params);
  AsyncLeaseKeepAliveResponse ParseResponse();

  etcd::Response Refresh();
  void CancelKeepAlive();
  bool Cancelled() const;

 private:
  etcdv3::ActionParameters& mutable_parameters();

  LeaseKeepAliveResponse reply;
  std::unique_ptr<
      ClientAsyncReaderWriter<LeaseKeepAliveRequest, LeaseKeepAliveResponse>>
      stream;

  LeaseKeepAliveRequest req;
  std::atomic_bool isCancelled;
  std::recursive_mutex protect_is_cancelled;

  friend class etcd::KeepAlive;
};

class AsyncAddMemberAction : public etcdv3::Action {
 public:
  AsyncAddMemberAction(etcdv3::ActionParameters&& params);
  AsyncMemberAddResponse ParseResponse();

 private:
  MemberAddResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<MemberAddResponse>> response_reader;
};

class AsyncListMemberAction : public etcdv3::Action {
 public:
  AsyncListMemberAction(etcdv3::ActionParameters&& params);
  AsyncMemberListResponse ParseResponse();

 private:
  MemberListResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<MemberListResponse>>
      response_reader;
};

class AsyncRemoveMemberAction : public etcdv3::Action {
 public:
  AsyncRemoveMemberAction(etcdv3::ActionParameters&& params);
  AsyncMemberRemoveResponse ParseResponse();

 private:
  MemberRemoveResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<MemberRemoveResponse>>
      response_reader;
};

class AsyncLeaseLeasesAction : public etcdv3::Action {
 public:
  AsyncLeaseLeasesAction(etcdv3::ActionParameters&& params);
  AsyncLeaseLeasesResponse ParseResponse();

 private:
  LeaseLeasesResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<LeaseLeasesResponse>>
      response_reader;
};

class AsyncLeaseRevokeAction : public etcdv3::Action {
 public:
  AsyncLeaseRevokeAction(etcdv3::ActionParameters&& params);
  AsyncLeaseRevokeResponse ParseResponse();

 private:
  LeaseRevokeResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<LeaseRevokeResponse>>
      response_reader;
};

class AsyncLeaseTimeToLiveAction : public etcdv3::Action {
 public:
  AsyncLeaseTimeToLiveAction(etcdv3::ActionParameters&& params);
  AsyncLeaseTimeToLiveResponse ParseResponse();

 private:
  LeaseTimeToLiveResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<LeaseTimeToLiveResponse>>
      response_reader;
};

class AsyncLockAction : public etcdv3::Action {
 public:
  AsyncLockAction(etcdv3::ActionParameters&& params);
  AsyncLockResponse ParseResponse();

 private:
  LockResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<LockResponse>> response_reader;
};

class AsyncObserveAction : public etcdv3::Action {
 public:
  AsyncObserveAction(etcdv3::ActionParameters&& params);
  AsyncObserveResponse ParseResponse();
  void waitForResponse();
  void CancelObserve();
  bool Cancelled() const;

 private:
  LeaderResponse reply;
  std::unique_ptr<ClientAsyncReader<LeaderResponse>> response_reader;
  std::atomic_bool isCancelled;
  std::mutex protect_is_cancelled;
};

class AsyncProclaimAction : public etcdv3::Action {
 public:
  AsyncProclaimAction(etcdv3::ActionParameters&& params);
  AsyncProclaimResponse ParseResponse();

 private:
  ProclaimResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<ProclaimResponse>> response_reader;
};

class AsyncPutAction : public etcdv3::Action {
 public:
  AsyncPutAction(etcdv3::ActionParameters&& params);
  AsyncPutResponse ParseResponse();

 private:
  PutResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<PutResponse>> response_reader;
};

class AsyncRangeAction : public etcdv3::Action {
 public:
  AsyncRangeAction(etcdv3::ActionParameters&& params);
  AsyncRangeResponse ParseResponse();

 private:
  RangeResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<RangeResponse>> response_reader;
};

class AsyncResignAction : public etcdv3::Action {
 public:
  AsyncResignAction(etcdv3::ActionParameters&& params);
  AsyncResignResponse ParseResponse();

 private:
  ResignResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<ResignResponse>> response_reader;
};

class AsyncSetAction : public etcdv3::Action {
 public:
  AsyncSetAction(etcdv3::ActionParameters&& params, bool create = false);
  AsyncTxnResponse ParseResponse();

 private:
  TxnResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<TxnResponse>> response_reader;
  bool isCreate;
};

class AsyncTxnAction : public etcdv3::Action {
 public:
  AsyncTxnAction(etcdv3::ActionParameters&& params,
                 etcdv3::Transaction const& tx);
  AsyncTxnResponse ParseResponse();

 private:
  TxnResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<TxnResponse>> response_reader;
};

class AsyncUnlockAction : public etcdv3::Action {
 public:
  AsyncUnlockAction(etcdv3::ActionParameters&& params);
  AsyncUnlockResponse ParseResponse();

 private:
  UnlockResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<UnlockResponse>> response_reader;
};

class AsyncUpdateAction : public etcdv3::Action {
 public:
  AsyncUpdateAction(etcdv3::ActionParameters&& params);
  AsyncTxnResponse ParseResponse();

 private:
  TxnResponse reply;
  std::unique_ptr<ClientAsyncResponseReader<TxnResponse>> response_reader;
};

class AsyncWatchAction : public etcdv3::Action {
 public:
  AsyncWatchAction(etcdv3::ActionParameters&& params);
  AsyncWatchResponse ParseResponse();
  void waitForResponse();
  void waitForResponse(std::function<void(etcd::Response)> callback);
  void CancelWatch();
  bool Cancelled() const;

 private:
  int64_t watch_id = -1;
  WatchResponse reply;
  std::unique_ptr<ClientAsyncReaderWriter<WatchRequest, WatchResponse>> stream;
  std::atomic_bool isCancelled;
};
}  // namespace etcdv3

#endif
