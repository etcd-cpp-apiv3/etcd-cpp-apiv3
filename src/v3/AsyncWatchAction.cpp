#include "etcd/v3/AsyncWatchAction.hpp"
#include "etcd/v3/action_constants.hpp"


using etcdserverpb::WatchCreateRequest;

etcdv3::AsyncWatchAction::AsyncWatchAction(
    etcdv3::ActionParameters && params)
  : etcdv3::Action(std::move(params))
{
  isCancelled.store(false);
  stream = parameters.watch_stub->AsyncWatch(&context,&cq_,(void*)etcdv3::WATCH_CREATE);

  WatchRequest watch_req;
  WatchCreateRequest watch_create_req;
  watch_create_req.set_key(parameters.key);
  watch_create_req.set_prev_kv(true);
  watch_create_req.set_start_revision(parameters.revision);

  if(parameters.withPrefix)
  {
    if (parameters.key.empty()) {
      watch_create_req.set_range_end(detail::string_plus_one(etcdv3::NUL));
    } else {
      watch_create_req.set_range_end(detail::string_plus_one(parameters.key));
    }
  }
  if(!parameters.range_end.empty()) {
    watch_create_req.set_range_end(parameters.range_end);
  }

  watch_req.mutable_create_request()->CopyFrom(watch_create_req);

  // wait "create" success (the stream becomes ready)
  void *got_tag;
  bool ok = false;
  if (cq_.Next(&got_tag, &ok) && ok && got_tag == (void *)etcdv3::WATCH_CREATE) {
    stream->Write(watch_req, (void *)etcdv3::WATCH_WRITE);
  } else {
    throw std::runtime_error("failed to create a watch connection");
  }

  // wait "write" (WatchCreateRequest) success, and start to read the first reply
  if (cq_.Next(&got_tag, &ok) && ok && got_tag == (void *)etcdv3::WATCH_WRITE) {
    stream->Read(&reply, (void*)this);
    this->watch_id = reply.watch_id();
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
    if(got_tag == (void *)etcdv3::WATCH_WRITE_CANCEL) {
      stream->WritesDone((void*)etcdv3::WATCH_WRITES_DONE);
      continue;
    }
    if(got_tag == (void*)etcdv3::WATCH_WRITES_DONE)
    {
      grpc::Status status;
      stream->Finish(&status, (void *)etcdv3::WATCH_FINISH);
      continue;
    }
    if (got_tag == (void *)etcdv3::WATCH_FINISH) {
      // shutdown
      cq_.Shutdown();
      // cancel on-the-fly calls
      context.TryCancel();
      break;
    }
    if(got_tag == (void*)this) // read tag
    {
      if (reply.canceled()) {
        // cancel on-the-fly calls, but don't shutdown the completion queue as there
        // are still a inflight call to finish
        context.TryCancel();
        // cq_.Shutdown();
        continue;
      }

      // we stop watch under two conditions:
      //
      // 1. watch for a future revision, return immediately with empty events set
      // 2. receive any effective events.
      if ((reply.created() && reply.header().revision() < parameters.revision) ||
          reply.events_size() > 0) {
        // leave a warning if the response is too large and been fragmented
        if (reply.fragment()) {
          std::cerr << "WARN: The response hasn't been fully received and parsed" << std::endl;
        }

        this->CancelWatch();
        continue;
      }
      // otherwise, start next round read-reply
      stream->Read(&reply, (void*)this);
      continue;
    }
    if(isCancelled.load()) {
      // invalid tag, and is cancelled
      break;
    }
  }
}

void etcdv3::AsyncWatchAction::CancelWatch()
{
  if (!isCancelled.exchange(true)) {
    WatchRequest cancel_req;
    cancel_req.mutable_cancel_request()->set_watch_id(this->watch_id);
    stream->Write(cancel_req, (void *)etcdv3::WATCH_WRITE_CANCEL);
    isCancelled.store(true);
  }
}

bool etcdv3::AsyncWatchAction::Cancelled() const {
  return isCancelled.load();
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
    if(got_tag == (void *)etcdv3::WATCH_WRITE_CANCEL) {
      stream->WritesDone((void*)etcdv3::WATCH_WRITES_DONE);
      continue;
    }
    if(got_tag == (void*)etcdv3::WATCH_WRITES_DONE)
    {
      grpc::Status status;
      stream->Finish(&status, (void *)etcdv3::WATCH_FINISH);
      continue;
    }
    if (got_tag == (void *)etcdv3::WATCH_FINISH) {
      // shutdown
      cq_.Shutdown();
      // cancel on-the-fly calls
      context.TryCancel();
      break;
    }
    if(got_tag == (void*)this) // read tag
    {
      if (reply.canceled()) {
        if (reply.compact_revision() != 0) {
          auto resp = ParseResponse();
          auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::high_resolution_clock::now() - start_timepoint);
          callback(etcd::Response(resp, duration));
        }
        // cancel on-the-fly calls, but don't shutdown the completion queue as there
        // are still a inflight call to finish
        context.TryCancel();
        // cq_.Shutdown();
        continue;
      }

      // for the callback case, we don't invoke callback immediately if watching
      // for a future revision, we wait until there are some effective events.
      if(reply.events_size())
      {
        auto resp = ParseResponse();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - start_timepoint);
        callback(etcd::Response(resp, duration));
        start_timepoint = std::chrono::high_resolution_clock::now();
      }
      stream->Read(&reply, (void*)this);
      continue;
    }
    if(isCancelled.load()) {
      // invalid tag, and is cancelled
      break;
    }
  }
}

etcdv3::AsyncWatchResponse etcdv3::AsyncWatchAction::ParseResponse()
{
  AsyncWatchResponse watch_resp;
  watch_resp.set_action(etcdv3::WATCH_ACTION);

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
