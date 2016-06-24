#include "v3/include/AsyncWatchAction.hpp"
#include "v3/include/action_constants.hpp"

using etcdserverpb::RangeRequest;
using etcdserverpb::RangeResponse;
using etcdserverpb::WatchCreateRequest;

etcdv3::AsyncWatchAction::AsyncWatchAction(std::string const & key, bool recursive, KV::Stub* stub_, Watch::Stub* watchServiceStub) 
{
  stream = watchServiceStub->AsyncWatch(&context,&cq_,(void*)this);

  WatchRequest watch_req;
  WatchCreateRequest watch_create_req;
  watch_create_req.set_key(key);

  std::string range_end(key); 
  if(recursive)
  {
    int ascii = (int)range_end[range_end.length()-1];
    range_end.back() = ascii+1;
    watch_create_req.set_range_end(range_end);
  }

  watch_req.mutable_create_request()->CopyFrom(watch_create_req);
  stream->Write(watch_req, (void*)this);
  stub_ = stub_;

}

etcdv3::AsyncWatchAction::AsyncWatchAction(std::string const & key, int fromIndex, bool recursive, KV::Stub* stub_, Watch::Stub* watchServiceStub) 
{
  stream = watchServiceStub->AsyncWatch(&context,&cq_,(void*)this);

  WatchRequest watch_req;
  WatchCreateRequest watch_create_req;
  watch_create_req.set_key(key);
  watch_create_req.set_start_revision(fromIndex);

  std::string range_end(key); 
  if(recursive)
  {
    int ascii = (int)range_end[range_end.length()-1];
    range_end.back() = ascii+1;
    watch_create_req.set_range_end(range_end);
  }


  watch_req.mutable_create_request()->CopyFrom(watch_create_req);
  stream->Write(watch_req, (void*)this);
  stub_ = stub_;
}

void etcdv3::AsyncWatchAction::waitForResponse() 
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

etcdv3::AsyncWatchResponse etcdv3::AsyncWatchAction::ParseResponse()
{

  AsyncWatchResponse watch_resp(reply);
  if(!status.ok())
  {
    watch_resp.error_code = status.error_code();
    watch_resp.error_message = status.error_message();
  }
  else
  { 
    watch_resp.ParseResponse();
  }
  return watch_resp;
}
