#include "etcd/v3/AsyncElectionResponse.hpp"

#include "etcd/v3/action_constants.hpp"

using etcdserverpb::ResponseOp;

void etcdv3::AsyncCampaignResponse::ParseResponse(CampaignResponse& reply) {
  index = reply.header().revision();

  auto const &leader = reply.leader();
  name = leader.name();
  value.kvs.set_key(leader.key());
  value.kvs.set_create_revision(leader.rev());
  value.kvs.set_lease(leader.lease());
}

void etcdv3::AsyncProclaimResponse::ParseResponse(ProclaimResponse& reply) {
  index = reply.header().revision();
}

void etcdv3::AsyncLeaderResponse::ParseResponse(LeaderResponse& reply) {
  index = reply.header().revision();

  value.kvs = reply.kv();
}

void etcdv3::AsyncObserveResponse::ParseResponse(LeaderResponse& reply) {
  index = reply.header().revision();

  value.kvs = reply.kv();
}

void etcdv3::AsyncResignResponse::ParseResponse(ResignResponse& reply) {
  index = reply.header().revision();
}
