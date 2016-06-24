#include "v3/include/AsyncWatchResponse.hpp"
#include "v3/include/action_constants.hpp"

using etcdserverpb::RangeRequest;
using etcdserverpb::RangeResponse;

etcdv3::AsyncWatchResponse::AsyncWatchResponse(WatchResponse& resp) 
{
  reply = resp;
}

etcdv3::AsyncWatchResponse::AsyncWatchResponse(const etcdv3::AsyncWatchResponse& other) 
{
  error_code = other.error_code;
  error_message = other.error_message;
  index = other.index;
  action = other.action;
  values = other.values;
  prev_values = other.prev_values;

}

etcdv3::AsyncWatchResponse& etcdv3::AsyncWatchResponse::operator=(const etcdv3::AsyncWatchResponse& other) 
{
  error_code = other.error_code;
  error_message = other.error_message;
  index = other.index;
  action = other.action;
  values = other.values;
  prev_values = other.prev_values;
  return *this;
}

void etcdv3::AsyncWatchResponse::ParseResponse()
{
  index = reply.header().revision();
  std::map<std::string, mvccpb::KeyValue> mapValue; 
  for(int cnt =0; cnt < reply.events_size(); cnt++)
  {
    auto event = reply.events(cnt);
    if(mvccpb::Event::EventType::Event_EventType_PUT == event.type())
    {
      if(event.has_kv())
      {
        auto kv = event.kv();
        if(kv.version() == 1)
        {
          action = etcdv3::CREATE_ACTION;
        }
        else
        {
          action = etcdv3::SET_ACTION;
        }
        mapValue.emplace(kv.key(), kv);
      }
    }
    else if(mvccpb::Event::EventType::Event_EventType_DELETE == event.type())
    {
      action = etcdv3::DELETE_ACTION;
    }
 
  }
  for(auto x: mapValue) 
  {
    values.push_back(x.second);
  }
}
