#include "etcd/v3/AsyncHeadResponse.hpp"
#include "etcd/v3/action_constants.hpp"


void etcdv3::AsyncHeadResponse::ParseResponse(RangeResponse& resp)
{
  cluster_id = resp.header().cluster_id();
  member_id = resp.header().member_id();
  index = resp.header().revision();
  raft_term = resp.header().raft_term();
}
