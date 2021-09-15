#include "etcd/v3/AsyncLockResponse.hpp"
#include "etcd/v3/action_constants.hpp"

void etcdv3::AsyncLockResponse::ParseResponse(LockResponse& resp)
{
  index = resp.header().revision();
  lock_key = resp.key();
}

void etcdv3::AsyncUnlockResponse::ParseResponse(UnlockResponse& resp)
{
  index = resp.header().revision();
}
