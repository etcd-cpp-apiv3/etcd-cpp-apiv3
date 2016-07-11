#include "v3/include/AsyncDeleteRangeResponse.hpp"
#include "v3/include/action_constants.hpp"


void etcdv3::AsyncDeleteRangeResponse::ParseResponse(std::string const& key, bool prefix, DeleteRangeResponse& resp)
{
  index = resp.header().revision();

  if(resp.prev_kvs_size() == 0)
  {
    error_code=100;
    error_message="Key not found";
  }
  else
  {
    action = etcdv3::DELETE_ACTION;
    //get all previous values
    for(int cnt=0; cnt < resp.prev_kvs_size(); cnt++)
    {
      values.push_back(resp.prev_kvs(cnt));
    }

    if(!prefix)
    {
      prev_value = values[0];
      value = values[0];
      value.clear_value();
      values.clear();
    }

  }
}
