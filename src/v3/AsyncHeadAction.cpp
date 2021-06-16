#include "etcd/v3/AsyncHeadAction.hpp"

#include <cstdlib>

#include "etcd/v3/action_constants.hpp"

using etcdserverpb::RangeRequest;

etcdv3::AsyncHeadAction::AsyncHeadAction(etcdv3::ActionParameters param)
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
  AsyncHeadResponse range_resp;
  if(!status.ok())
  {
    range_resp.set_error_code(status.error_code());
    range_resp.set_error_message(status.error_message());
  }
  else
  {
    range_resp.ParseResponse(reply, parameters.withPrefix || !parameters.range_end.empty());
  }
  return range_resp;
}
