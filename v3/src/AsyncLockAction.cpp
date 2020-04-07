#include "v3/include/AsyncLockAction.hpp"
#include "v3/include/action_constants.hpp"

using v3lockpb::LockRequest;
using v3lockpb::UnlockRequest;

etcdv3::AsyncLockAction::AsyncLockAction(ActionParameters param)
  : etcdv3::Action(param) 
{
  LockRequest lock_request;
  lock_request.set_name(parameters.key);

  response_reader = parameters.lock_stub->AsyncLock(&context, lock_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncLockResponse etcdv3::AsyncLockAction::ParseResponse()
{
  AsyncLockResponse lock_resp;
  
  if(!status.ok())
  {
    std::cout << "lock error message is: " << status.error_message() << std::endl;
    lock_resp.set_error_code(status.error_code());
    lock_resp.set_error_message(status.error_message());
  }
  else
  { 
    lock_resp.ParseResponse(reply);
    lock_resp.set_action(etcdv3::LOCK_ACTION);
  }

  return lock_resp;
}

etcdv3::AsyncUnlockAction::AsyncUnlockAction(ActionParameters param)
  : etcdv3::Action(param) 
{
  UnlockRequest unlock_request;
  unlock_request.set_key(parameters.key);

  response_reader = parameters.lock_stub->AsyncUnlock(&context, unlock_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncUnlockResponse etcdv3::AsyncUnlockAction::ParseResponse()
{
  AsyncUnlockResponse unlock_resp;
  
  if(!status.ok())
  {
    std::cout << "unlock error message is: " << status.error_message() << std::endl;
    unlock_resp.set_error_code(status.error_code());
    unlock_resp.set_error_message(status.error_message());
  }
  else
  { 
    unlock_resp.ParseResponse(reply);
    unlock_resp.set_action(etcdv3::UNLOCK_ACTION);
  }
    
  return unlock_resp;
}
