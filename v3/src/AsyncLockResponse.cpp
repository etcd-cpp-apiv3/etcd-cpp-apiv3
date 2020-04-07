#include "v3/include/AsyncLockResponse.hpp"
#include "v3/include/action_constants.hpp"


void etcdv3::AsyncLockResponse::ParseResponse(LockResponse& resp)
{
  index = resp.header().revision();
  lock_key = resp.key();
}

void etcdv3::AsyncUnlockResponse::ParseResponse(UnlockResponse& resp)
{
  index = resp.header().revision();
}
