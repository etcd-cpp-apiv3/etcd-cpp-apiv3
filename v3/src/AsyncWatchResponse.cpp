#include "v3/include/AsyncWatchResponse.hpp"

using etcdserverpb::RangeRequest;
using etcdserverpb::RangeResponse;


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

void etcdv3::AsyncWatchResponse::waitForResponse() 
{
  void* got_tag;
  bool ok = false;    

  stream->Read(&reply, (void*)3);
  while(cq_.Next(&got_tag, &ok))
  {
    if(got_tag == (void*)3)
    {
      if(reply.events_size())
      {
        stream->WritesDone((void*)this);
        cq_.Next(&got_tag, &ok);
        break;
      } 
      else
      {
        stream->Read(&reply, (void*)3);
      }
    }   
  }
}

etcdv3::AsyncWatchResponse& etcdv3::AsyncWatchResponse::ParseResponse()
{
  index = reply.header().revision();

  mvccpb::KeyValue kv;
  std::map<std::string, mvccpb::KeyValue> mapValue;

  for(int cnt =0; cnt < reply.events_size(); cnt++)
  {
    auto event = reply.events(cnt);
    if(mvccpb::Event::EventType::Event_EventType_PUT == event.type())
    {
      if(event.has_kv())
      {
        kv = event.kv();
        if(kv.version() == 1)
        {
          action = "create";
        }
        else
        {
          action = "set";
        }
        //values.push_back(kv);
        mapValue.emplace(kv.key(), kv);
      }
    }
    else if(mvccpb::Event::EventType::Event_EventType_DELETE == event.type())
    {
      action = "delete";
    }
    //get previous value index - 1
    RangeRequest get_request;
    get_request.set_key(kv.key());
    get_request.set_revision(index - 1);

    RangeResponse response;
    ClientContext ctx;

    Status result = stub_->Range(&ctx, get_request, &response);
    if (result.ok()) 
    {
      for(int cnt=0; cnt < response.kvs_size(); cnt++)
      {
        prev_values.push_back(response.kvs(cnt));
      }
    } 
  }
  for(auto x: mapValue) 
  {
    values.push_back(x.second);
  }

  return *this;
}
