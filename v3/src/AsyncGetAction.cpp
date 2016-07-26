#include "v3/include/AsyncGetAction.hpp"
#include "v3/include/action_constants.hpp"

using etcdserverpb::RangeRequest;

etcdv3::AsyncGetAction::AsyncGetAction(etcdv3::ActionParameters param)
  : etcdv3::Action(param)
{
  RangeRequest get_request;
  get_request.set_key(parameters.key);
  if(parameters.withPrefix)
  {
    std::string range_end(parameters.key); 
    int ascii = (int)range_end[range_end.length()-1];
    range_end.back() = ascii+1;

    get_request.set_range_end(range_end);
    get_request.set_sort_target(RangeRequest::SortTarget::RangeRequest_SortTarget_KEY);
    get_request.set_sort_order(RangeRequest::SortOrder::RangeRequest_SortOrder_ASCEND);
  }   
  response_reader = parameters.kv_stub->AsyncRange(&context,get_request,&cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncRangeResponse etcdv3::AsyncGetAction::ParseResponse()
{
  AsyncRangeResponse range_resp;
  if(!status.ok())
  {
    range_resp.set_error_code(status.error_code());
    range_resp.set_error_message(status.error_message());
  }
  else
  { 
    range_resp.ParseResponse(reply, parameters.withPrefix);
  }
  return range_resp;
}
