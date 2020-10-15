#include "etcd/v3/AsyncWatchAction.hpp"
#include "etcd/v3/action_constants.hpp"


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

  // wait "create" success (the stream becomes ready)
  void *got_tag;
  bool ok = false;
  if (cq_.Next(&got_tag, &ok) && ok && got_tag == (void *)"create") {
    stream->Write(watch_req, (void *)"write");
  } else {
    throw std::runtime_error("failed to create a watch connection");
  }

  // wait "write" (WatchCreateRequest) success, and start to read the first reply
  if (cq_.Next(&got_tag, &ok) && ok && got_tag == (void *)"write") {
    stream->Read(&reply, (void*)this);
  } else {
    throw std::runtime_error("failed to write WatchCreateRequest to server");
  }
}

void etcdv3::AsyncWatchAction::waitForResponse() 
{
  void* got_tag;
  bool ok = false;

  while(cq_.Next(&got_tag, &ok))
  {
    if(ok == false)
    {
      break;
    }
    if(got_tag == (void*)"writes done") {
      isCancelled = true;
      cq_.Shutdown();
      break;
    }
    if(got_tag == (void*)this) // read tag
    {
      if ((reply.created() && reply.header().revision() < parameters.revision) ||
          reply.events_size() > 0) {
        // we stop watch under two conditions:
        //
        // 1. watch for a future revision, return immediately with empty events set
        // 2. receive any effective events.
        stream->WritesDone((void*)"writes done");

        // leave a warning if the response is too large and been fragmented
        if (reply.fragment()) {
          std::cerr << "WARN: The response hasn't been fully received and parsed" << std::endl;
        }
      }
      else
      {
        // otherwise, start next round read-reply
        stream->Read(&reply, (void*)this);
      } 
    }  
  }
}

void etcdv3::AsyncWatchAction::CancelWatch()
{
  std::lock_guard<std::mutex> scope_lock(this->protect_is_cancalled);
  if(isCancelled == false)
  {
    stream->WritesDone((void*)"writes done");
  }
  isCancelled = true;
}

bool etcdv3::AsyncWatchAction::Cancelled() const {
  return isCancelled;
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
      cq_.Shutdown();
      break;
    }
    else if(got_tag == (void*)this) // read tag
    {
      if(reply.events_size())
      {
        // for the callback case, we don't stop immediately if watching for a future revison,
        // we wait until there are some expected events.
        auto resp = ParseResponse();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - start_timepoint);
        callback(etcd::Response(resp, duration)); 
        start_timepoint = std::chrono::high_resolution_clock::now();
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
