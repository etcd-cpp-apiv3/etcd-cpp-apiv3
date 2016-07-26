#include "v3/include/AsyncWatchResponse.hpp"
#include "v3/include/action_constants.hpp"

void etcdv3::AsyncWatchResponse::ParseResponse(WatchResponse& reply)
{
  index = reply.header().revision();
  for(int cnt =0; cnt < reply.events_size(); cnt++)
  {
    auto event = reply.events(cnt);
    if(mvccpb::Event::EventType::Event_EventType_PUT == event.type())
    {
      if(event.kv().version() == 1)
      {
        action = etcdv3::CREATE_ACTION;
      }
      else
      {
        action = etcdv3::SET_ACTION;
      }
      value.kvs = event.kv();       

    }
    else if(mvccpb::Event::EventType::Event_EventType_DELETE == event.type())
    {
      action = etcdv3::DELETE_ACTION;
      value.kvs = event.kv();
    } 
    if(event.has_prev_kv())
    {
      prev_value.kvs = event.prev_kv();   
    }
    // just store the first occurence of the key in values.
    // this is done so tas client will not need to change their behaviour.
    // break immediately
    break;
  }
}
