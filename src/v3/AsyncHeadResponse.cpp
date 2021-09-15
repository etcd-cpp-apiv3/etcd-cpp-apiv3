#include "etcd/v3/AsyncHeadResponse.hpp"
#include "etcd/v3/action_constants.hpp"


void etcdv3::AsyncHeadResponse::ParseResponse(RangeResponse& resp)
{
  index = resp.header().revision();
}
