#include "etcd/v3/AsyncGetAction.hpp"

#include <cstdlib>

#include "etcd/v3/action_constants.hpp"

using etcdserverpb::RangeRequest;

static std::string string_plus_one(std::string const &value) {
  char *s = static_cast<char *>(calloc(value.size() + 1, sizeof(char)));
  std::memcpy(s, value.c_str(), value.size());
  for (int i = value.size() - 1; i >= 0; --i) {
    if (static_cast<unsigned char>(s[i]) < 0xff) {
      s[i] = s[i] + 1;
      std::string ret = std::string(s, i + 1);
      free(s);
      return ret;
    }
  }
  // see: noPrefixEnd in etcd
  return {"\0"};
}

etcdv3::AsyncGetAction::AsyncGetAction(etcdv3::ActionParameters param)
  : etcdv3::Action(param)
{
  RangeRequest get_request;
  if (parameters.key.empty()) {
    get_request.set_key("\0");
  } else {
    get_request.set_key(parameters.key);
  }
  get_request.set_limit(param.limit);
  if(parameters.withPrefix)
  {
    if (parameters.key.empty()) {
      get_request.set_range_end(string_plus_one("\0"));
    } else {
      get_request.set_range_end(string_plus_one(parameters.key));
    }
    get_request.set_sort_order(RangeRequest::SortOrder::RangeRequest_SortOrder_NONE);
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
