#include "v3/include/AsyncWatchAction.hpp"
#include "v3/include/action_constants.hpp"


using etcdserverpb::RangeRequest;
using etcdserverpb::RangeResponse;
using etcdserverpb::WatchCreateRequest;

etcdv3::AsyncWatchAction::AsyncWatchAction(std::string const & key, bool recursive, KV::Stub* stub_, Watch::Stub* watchServiceStub) 
{
  std::cout << "AsyncWatchAction create start" << std::endl;
  stream = watchServiceStub->AsyncWatch(&context,&cq_,(void*)"create");

  WatchRequest watch_req;
  WatchCreateRequest watch_create_req;
  watch_create_req.set_key(key);

  std::string range_end(key); 
  prefix = recursive;
  if(recursive)
  {
    int ascii = (int)range_end[range_end.length()-1];
    range_end.back() = ascii+1;
    watch_create_req.set_range_end(range_end);
  }

  watch_req.mutable_create_request()->CopyFrom(watch_create_req);
  stream->Write(watch_req, (void*)"write");
  stream->Read(&reply, (void*)this);
  stub_ = stub_;
  std::cout << "AsyncWatchAction create end" << std::endl;

}

etcdv3::AsyncWatchAction::AsyncWatchAction(std::string const & key, int fromIndex, bool recursive, KV::Stub* stub_, Watch::Stub* watchServiceStub) 
{
  stream = watchServiceStub->AsyncWatch(&context,&cq_,(void*)1);

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
  stream->Write(watch_req, (void*)1);
  stream->Read(&reply, (void*)this);
  stub_ = stub_;
}

void etcdv3::AsyncWatchAction::WatchReq(std::string const & key) 
{
  WatchRequest watch_req;
  WatchCreateRequest watch_create_req;
  watch_create_req.set_key(key);

  watch_req.mutable_create_request()->CopyFrom(watch_create_req);
  stream->Write(watch_req, (void*)1);
  stream->Read(&reply, (void*)this);
}


void etcdv3::AsyncWatchAction::waitForResponse() 
{
  void* got_tag;
  bool ok = false;    

  while(cq_.Next(&got_tag, &ok))
  {
    if(got_tag == (void*)this) // read tag
    {
      if(reply.events_size())
      {
        stream->WritesDone((void*)this);
        cq_.Next(&got_tag, &ok);
        break;
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
  std::cout << "cancel watch"<< std::endl;
  stream->WritesDone((void*)"writes done");
}

void etcdv3::AsyncWatchAction::waitForResponse(std::function<void(etcd::Response)> callback) 
{
  std::cout << "waitForResponse start" << std::endl;
  void* got_tag;
  bool ok = false;    

  while(cq_.Next(&got_tag, &ok))
  {
    if(ok == false)
    {
      break;
    }
    std::cout << "ok status: " << ok << std::endl;
    if(got_tag == (void*)"writes done")
    {
      std::cout << "writes done" << std::endl;
    }
    else if(got_tag == (void*)this) // read tag
    {
      std::cout << "read tag" << std::endl;
      std::cout << "events size: "<< reply.events_size() << std::endl;
      if(reply.events_size())
      {
        auto resp = ParseResponse();
        callback(resp); 
        std::cout << "events received try to read again" << std::endl;
      }
      std::cout << " read again" << std::endl;
      stream->Read(&reply, (void*)this);
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
