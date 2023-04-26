#include "etcd/v3/AsyncRangeAction.hpp"

#include <cstdlib>

#include "etcd/v3/action_constants.hpp"

using etcdserverpb::RangeRequest;

etcdv3::AsyncRangeAction::AsyncRangeAction(
    etcdv3::ActionParameters && params)
  : etcdv3::Action(std::move(params))
{
  RangeRequest get_request;
  if (!parameters.withPrefix) {
    get_request.set_key(parameters.key);
  } else {
    if (parameters.key.empty()) {
      // see: WithFromKey in etcdv3/client
      get_request.set_key(etcdv3::NUL);
      get_request.set_range_end(etcdv3::NUL);
    } else {
      get_request.set_key(parameters.key);
      get_request.set_range_end(detail::string_plus_one(parameters.key));
    }
  }
  if(!parameters.range_end.empty()) {
    get_request.set_range_end(parameters.range_end);
  }
  if(parameters.revision > 0) {
    get_request.set_revision(parameters.revision);
  }

  get_request.set_limit(parameters.limit);
  get_request.set_sort_order(RangeRequest::SortOrder::RangeRequest_SortOrder_NONE);

  // set keys_only and count_only
  get_request.set_keys_only(params.keys_only);
  get_request.set_count_only(params.count_only);

  response_reader = parameters.kv_stub->AsyncRange(&context,get_request,&cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncRangeResponse etcdv3::AsyncRangeAction::ParseResponse()
{
  AsyncRangeResponse range_resp;
  range_resp.set_action(etcdv3::GET_ACTION);

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
