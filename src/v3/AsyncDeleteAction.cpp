#include "etcd/v3/AsyncDeleteAction.hpp"
#include "etcd/v3/action_constants.hpp"

using etcdserverpb::DeleteRangeRequest;

etcdv3::AsyncDeleteAction::AsyncDeleteAction(
    ActionParameters && params)
  : etcdv3::Action(std::move(params))
{
  DeleteRangeRequest del_request;
  if (!parameters.withPrefix) {
    del_request.set_key(parameters.key);
  } else {
    if (parameters.key.empty()) {
      // see: WithFromKey in etcdv3/client
      del_request.set_key(etcdv3::NUL);
      del_request.set_range_end(etcdv3::NUL);
    } else {
      del_request.set_key(parameters.key);
      del_request.set_range_end(detail::string_plus_one(parameters.key));
    }
  }
  if(!parameters.range_end.empty()) {
    del_request.set_range_end(parameters.range_end);
  }

  del_request.set_prev_kv(true);

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
