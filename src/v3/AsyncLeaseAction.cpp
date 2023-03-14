#include "etcd/v3/AsyncLeaseAction.hpp"

#include "etcd/Response.hpp"
#include "etcd/v3/action_constants.hpp"
#include "etcd/v3/Transaction.hpp"

#include <grpcpp/support/status.h>

using etcdserverpb::LeaseGrantRequest;
using etcdserverpb::LeaseRevokeRequest;
using etcdserverpb::LeaseCheckpointRequest;
using etcdserverpb::LeaseKeepAliveRequest;
using etcdserverpb::LeaseTimeToLiveRequest;
using etcdserverpb::LeaseLeasesRequest;

etcdv3::AsyncLeaseGrantAction::AsyncLeaseGrantAction(
    etcdv3::ActionParameters && params)
  : etcdv3::Action(std::move(params))
{
  LeaseGrantRequest leasegrant_request;
  leasegrant_request.set_ttl(parameters.ttl);
  // If ID is set to 0, etcd will choose an ID.
  leasegrant_request.set_id(parameters.lease_id);

  response_reader = parameters.lease_stub->AsyncLeaseGrant(&context, leasegrant_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncLeaseGrantResponse etcdv3::AsyncLeaseGrantAction::ParseResponse()
{
  AsyncLeaseGrantResponse lease_resp;
  lease_resp.set_action(etcdv3::LEASEGRANT);

  if (!status.ok()) {
    lease_resp.set_error_code(status.error_code());
    lease_resp.set_error_message(status.error_message());
  } else { 
    lease_resp.ParseResponse(reply);
  }
  return lease_resp;
}

etcdv3::AsyncLeaseRevokeAction::AsyncLeaseRevokeAction(
    etcdv3::ActionParameters && params)
  : etcdv3::Action(std::move(params))
{
  LeaseRevokeRequest leaserevoke_request;
  leaserevoke_request.set_id(parameters.lease_id);

  response_reader = parameters.lease_stub->AsyncLeaseRevoke(&context, leaserevoke_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncLeaseRevokeResponse etcdv3::AsyncLeaseRevokeAction::ParseResponse()
{
  AsyncLeaseRevokeResponse lease_resp;
  lease_resp.set_action(etcdv3::LEASEREVOKE);

  if (!status.ok()) {
    lease_resp.set_error_code(status.error_code());
    lease_resp.set_error_message(status.error_message());
  } else { 
    lease_resp.ParseResponse(reply);
  }
  return lease_resp;
}

etcdv3::AsyncLeaseKeepAliveAction::AsyncLeaseKeepAliveAction(
    etcdv3::ActionParameters && params)
  : etcdv3::Action(std::move(params))
{
  isCancelled = false;
  stream = parameters.lease_stub->AsyncLeaseKeepAlive(&context, &cq_, (void*)etcdv3::KEEPALIVE_CREATE);

  void *got_tag = nullptr;
  bool ok = false;
  if (cq_.Next(&got_tag, &ok) && ok && got_tag == (void *)etcdv3::KEEPALIVE_CREATE) {
    // ok
  } else {
    throw std::runtime_error("Failed to create a lease keep-alive connection");
  }
}

etcdv3::AsyncLeaseKeepAliveResponse etcdv3::AsyncLeaseKeepAliveAction::ParseResponse()
{
  AsyncLeaseKeepAliveResponse lease_resp;
  lease_resp.set_action(etcdv3::LEASEKEEPALIVE);

  if (!status.ok()) {
    lease_resp.set_error_code(status.error_code());
    lease_resp.set_error_message(status.error_message());
  } else { 
    lease_resp.ParseResponse(reply);
  }
  return lease_resp;
}

etcd::Response etcdv3::AsyncLeaseKeepAliveAction::Refresh()
{
  std::lock_guard<std::recursive_mutex> scope_lock(this->protect_is_cancelled);

  auto start_timepoint = std::chrono::high_resolution_clock::now();
  if (isCancelled) {
    status = grpc::Status::CANCELLED;
    return etcd::Response(ParseResponse(), etcd::detail::duration_till_now(start_timepoint));
  }

  LeaseKeepAliveRequest leasekeepalive_request;
  leasekeepalive_request.set_id(parameters.lease_id);

  void *got_tag = nullptr;
  bool ok = false;

  if (parameters.has_grpc_timeout()) {
    stream->Write(leasekeepalive_request, (void *)etcdv3::KEEPALIVE_WRITE);
    // wait write finish
    switch (cq_.AsyncNext(&got_tag, &ok, parameters.grpc_deadline())) {
      case CompletionQueue::NextStatus::TIMEOUT: {
        status = grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED, "gRPC timeout during keep alive write");
        break;
      }
      case CompletionQueue::NextStatus::SHUTDOWN: {
        status = grpc::Status(grpc::StatusCode::UNAVAILABLE, "gRPC already shutdown during keep alive write");
        break;
      }
      case CompletionQueue::NextStatus::GOT_EVENT: {
        if (!ok || got_tag != (void *)etcdv3::KEEPALIVE_WRITE) {
          return etcd::Response(grpc::StatusCode::ABORTED, "Failed to create a lease keep-alive connection: write not ok or invalid tag");
        }
      }
    }
    if (!status.ok()) {
      this->CancelKeepAlive();
      return etcd::Response(ParseResponse(), etcd::detail::duration_till_now(start_timepoint));
    }

    stream->Read(&reply, (void*)etcdv3::KEEPALIVE_READ);
    // wait read finish
    switch (cq_.AsyncNext(&got_tag, &ok, parameters.grpc_deadline())) {
      case CompletionQueue::NextStatus::TIMEOUT: {
        status = grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED, "gRPC timeout during keep alive read");
        break;
      }
      case CompletionQueue::NextStatus::SHUTDOWN: {
        status = grpc::Status(grpc::StatusCode::UNAVAILABLE, "gRPC already shutdown during keep alive read");
        break;
      }
      case CompletionQueue::NextStatus::GOT_EVENT: {
        if (ok && got_tag == (void *)etcdv3::KEEPALIVE_READ) {
          return etcd::Response(ParseResponse(), etcd::detail::duration_till_now(start_timepoint));
        }
        break;
      }
    }
    this->CancelKeepAlive();
    return etcd::Response(grpc::StatusCode::ABORTED, "Failed to create a lease keep-alive connection: read not ok or invalid tag");
  } else {
    stream->Write(leasekeepalive_request, (void *)etcdv3::KEEPALIVE_WRITE);
    // wait write finish
    if (cq_.Next(&got_tag, &ok) && ok && got_tag == (void *)etcdv3::KEEPALIVE_WRITE) {
      stream->Read(&reply, (void*)etcdv3::KEEPALIVE_READ);
      // wait read finish
      if (cq_.Next(&got_tag, &ok) && ok && got_tag == (void *)etcdv3::KEEPALIVE_READ) {
        return etcd::Response(ParseResponse(), etcd::detail::duration_till_now(start_timepoint));
      }
    }
    this->CancelKeepAlive();
    return etcd::Response(grpc::StatusCode::ABORTED, "Failed to create a lease keep-alive connection: read not ok or invalid tag");
  }
}

void etcdv3::AsyncLeaseKeepAliveAction::CancelKeepAlive()
{
  std::lock_guard<std::recursive_mutex> scope_lock(this->protect_is_cancelled);
  if(isCancelled == false)
  {
    isCancelled = true;

    void *got_tag = nullptr;
    bool ok = false;

    stream->WritesDone((void*)etcdv3::KEEPALIVE_DONE);
    if (cq_.Next(&got_tag, &ok) && ok && got_tag == (void *)etcdv3::KEEPALIVE_DONE) {
      // ok
    } else {
      std::cerr << "Failed to mark a lease keep-alive connection as DONE: "
                << context.debug_error_string() << std::endl;
    }

    stream->Finish(&status, (void *)KEEPALIVE_FINISH);
    if (cq_.Next(&got_tag, &ok) && ok && got_tag == (void *)KEEPALIVE_FINISH) {
      // ok
    } else {
      std::cerr << "Failed to finish a lease keep-alive connection: "
                << status.error_message()
                << ", " << context.debug_error_string() << std::endl;
    }

    // cancel on-the-fly calls
    context.TryCancel();

    cq_.Shutdown();
  }
}

bool etcdv3::AsyncLeaseKeepAliveAction::Cancelled() const
{
  return isCancelled;
}

etcdv3::ActionParameters& etcdv3::AsyncLeaseKeepAliveAction::mutable_parameters() {
  return this->parameters;
}

etcdv3::AsyncLeaseTimeToLiveAction::AsyncLeaseTimeToLiveAction(
    etcdv3::ActionParameters && params)
  : etcdv3::Action(std::move(params))
{
  LeaseTimeToLiveRequest leasetimetolive_request;
  leasetimetolive_request.set_id(parameters.lease_id);
  // FIXME: unsupported parameters: "keys"
  // leasetimetolive_request.set_keys(parameters.keys);

  response_reader = parameters.lease_stub->AsyncLeaseTimeToLive(&context, leasetimetolive_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncLeaseTimeToLiveResponse etcdv3::AsyncLeaseTimeToLiveAction::ParseResponse()
{
  AsyncLeaseTimeToLiveResponse lease_resp;
  lease_resp.set_action(etcdv3::LEASETIMETOLIVE);

  if (!status.ok()) {
    lease_resp.set_error_code(status.error_code());
    lease_resp.set_error_message(status.error_message());
  } else { 
    lease_resp.ParseResponse(reply);
  }
  return lease_resp;
}

etcdv3::AsyncLeaseLeasesAction::AsyncLeaseLeasesAction(
    etcdv3::ActionParameters && params)
  : etcdv3::Action(std::move(params))
{
  LeaseLeasesRequest leaseleases_request;

  response_reader = parameters.lease_stub->AsyncLeaseLeases(&context, leaseleases_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncLeaseLeasesResponse etcdv3::AsyncLeaseLeasesAction::ParseResponse()
{
  AsyncLeaseLeasesResponse lease_resp;
  lease_resp.set_action(etcdv3::LEASELEASES);

  if (!status.ok()) {
    lease_resp.set_error_code(status.error_code());
    lease_resp.set_error_message(status.error_message());
  } else { 
    lease_resp.ParseResponse(reply);
  }
  return lease_resp;
}
