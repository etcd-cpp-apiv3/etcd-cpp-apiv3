#include "v3/include/AsyncWatchAction.hpp"
#include "v3/include/action_constants.hpp"


using etcdserverpb::RangeRequest;
using etcdserverpb::RangeResponse;
using etcdserverpb::WatchCreateRequest;

etcdv3::AsyncWatchAction::AsyncWatchAction(etcdv3::ActionParameters param)
  : etcdv3::Action(param) 
{
  isCancelled = false;
  stream = parameters.watch_stub->AsyncWatch(&context,&cq_,(void*)"create");

  WatchRequest watch_req;
  WatchCreateRequest watch_create_req;
  watch_create_req.set_key(parameters.key);
  watch_create_req.set_prev_kv(true);
  watch_create_req.set_start_revision(parameters.revision);

  if(parameters.withPrefix)
  {
    std::string range_end(parameters.key); 
    int ascii = (int)range_end[range_end.length()-1];
    range_end.back() = ascii+1;
    watch_create_req.set_range_end(range_end);
  }

  watch_req.mutable_create_request()->CopyFrom(watch_create_req);
  stream->Write(watch_req, (void*)"write");
  stream->Read(&reply, (void*)this);
}


void etcdv3::AsyncWatchAction::waitForResponse() 
{
  void* got_tag;
  bool ok = false;    
  
  while(cq_.Next(&got_tag, &ok))
  {
    if(ok == false || (got_tag == (void*)"writes done"))
    {
      break;
    }
    if(got_tag == (void*)this) // read tag
    {
      if(reply.events_size())
      {
        stream->WritesDone((void*)"writes done");
      }
      else
      {
        stream->Read(&reply, (void*)this);
      } 
    }  
  }
}

void etcdv3::AsyncWatchAction::CancelWatch()
{
  if(isCancelled == false)
  {
    stream->WritesDone((void*)"writes done");
  }
}

void etcdv3::AsyncWatchAction::waitForResponse(std::function<void(etcd::Response)> callback) 
{
  void* got_tag;
  bool ok = false;    

  while(cq_.Next(&got_tag, &ok))
  {
    if(ok == false)
    {
      break;
    }
    if(got_tag == (void*)"writes done")
    {
      isCancelled = true;
    }
    else if(got_tag == (void*)this) // read tag
    {
      if(reply.events_size())
      {
        auto resp = ParseResponse();
        callback(resp); 
      }
      stream->Read(&reply, (void*)this);
    }     
  }
}

etcdv3::AsyncWatchResponse etcdv3::AsyncWatchAction::ParseResponse()
{

  AsyncWatchResponse watch_resp;
  if(!status.ok())
  {
    watch_resp.set_error_code(status.error_code());
    watch_resp.set_error_message(status.error_message());
  }
  else
  { 
    watch_resp.ParseResponse(reply);
  }
  return watch_resp;
}
