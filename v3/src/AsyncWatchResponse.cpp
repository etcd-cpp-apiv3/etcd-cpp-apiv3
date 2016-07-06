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
  std::map<std::string, mvccpb::KeyValue> mapPrevValue; 
  std::cout << "events size: " << reply.events_size() <<std::endl;
  for(int cnt =0; cnt < reply.events_size(); cnt++)
  {
    auto event = reply.events(cnt);
    const mvccpb::KeyValue& kv = event.kv();
    if(mvccpb::Event::EventType::Event_EventType_PUT == event.type())
    {
      if(kv.version() == 1)
      {
        action = etcdv3::CREATE_ACTION;
      }
      else
      {
        action = etcdv3::SET_ACTION;
      }
      // just store the first occurence of the key in values.
      // this is done so tas client will not need to change their behaviour.
      // and then break immediately
      mapValue.emplace(kv.key(), kv);  
      break;      

    }
    else if(mvccpb::Event::EventType::Event_EventType_DELETE == event.type())
    {
      action = etcdv3::DELETE_ACTION;
      // just store the first occurence of the key in values.
      // this is done so tas client will not need to change their behaviour.
      // break immediately
      mapValue.emplace(kv.key(), kv); 
      break;
    } 

    if(event.has_prev_kv())
    {
      
      auto kv = event.prev_kv();
      std::cout << "previous value of key: " << kv.key() << " is " << kv.value() << std::endl;
      mapPrevValue.emplace(kv.key(),kv);     
    }
    
  }
  for(auto x: mapPrevValue) 
  {
    prev_values.push_back(x.second);
  }
  for(auto x: mapValue) 
  {
    values.push_back(x.second);
  }
}
