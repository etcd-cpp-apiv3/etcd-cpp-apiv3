#include "etcd/v3/AsyncDeleteAction.hpp"
#include "etcd/v3/action_constants.hpp"

using etcdserverpb::DeleteRangeRequest;

etcdv3::AsyncDeleteAction::AsyncDeleteAction(
    ActionParameters const &param)
  : etcdv3::Action(param)
{
  DeleteRangeRequest del_request;
  del_request.set_key(parameters.key);
  del_request.set_prev_kv(true);
  if(parameters.withPrefix)
  {
    if (parameters.key.empty()) {
      del_request.set_range_end(detail::string_plus_one(etcdv3::NUL));
    } else {
      del_request.set_range_end(detail::string_plus_one(parameters.key));
    }
  }
  if(!parameters.range_end.empty()) {
    del_request.set_range_end(parameters.range_end);
  }

  response_reader = parameters.kv_stub->AsyncDeleteRange(&context, del_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncDeleteResponse etcdv3::AsyncDeleteAction::ParseResponse()
{
  AsyncDeleteResponse del_resp;
  del_resp.set_action(etcdv3::DELETE_ACTION);

  if(!status.ok())
  {
    del_resp.set_error_code(status.error_code());
    del_resp.set_error_message(status.error_message());
  }
  else
  {
    del_resp.ParseResponse(parameters.key, parameters.withPrefix || !parameters.range_end.empty(), reply);
  }

  return del_resp;
}
