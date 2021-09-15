#include "etcd/v3/AsyncHeadAction.hpp"

#include <cstdlib>

#include "etcd/v3/action_constants.hpp"

using etcdserverpb::RangeRequest;

etcdv3::AsyncHeadAction::AsyncHeadAction(
    etcdv3::ActionParameters const &param)
  : etcdv3::Action(param)
{
  RangeRequest get_request;
  get_request.set_key(etcdv3::NUL);
  get_request.set_limit(1);
  response_reader = parameters.kv_stub->AsyncRange(&context,get_request,&cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncHeadResponse etcdv3::AsyncHeadAction::ParseResponse()
{
  AsyncHeadResponse head_resp;
  head_resp.set_action(etcdv3::GET_ACTION);

  if(!status.ok())
  {
    head_resp.set_error_code(status.error_code());
    head_resp.set_error_message(status.error_message());
  }
  else
  {
    head_resp.ParseResponse(reply);
  }
  return head_resp;
}
