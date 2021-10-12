#include "etcd/v3/AsyncPutAction.hpp"
#include "etcd/v3/action_constants.hpp"

using etcdserverpb::PutRequest;

etcdv3::AsyncPutAction::AsyncPutAction(
    ActionParameters const &param)
  : etcdv3::Action(param)
{
  PutRequest put_request;
  put_request.set_key(parameters.key);
  put_request.set_value(parameters.value);
  put_request.set_lease(parameters.lease_id);
  put_request.set_prev_kv(true);

  response_reader = parameters.kv_stub->AsyncPut(&context, put_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncPutResponse etcdv3::AsyncPutAction::ParseResponse()
{
  AsyncPutResponse put_resp;
  put_resp.set_action(etcdv3::PUT_ACTION);

  if(!status.ok())
  {
    put_resp.set_error_code(status.error_code());
    put_resp.set_error_message(status.error_message());
  }
  else
  {
    put_resp.ParseResponse(reply);
  }

  return   put_resp;
}
