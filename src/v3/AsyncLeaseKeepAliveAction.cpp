#include "etcd/v3/AsyncLeaseKeepAliveAction.hpp"

#include "etcd/v3/Transaction.hpp"
#include "etcd/v3/action_constants.hpp"

using etcdserverpb::LeaseKeepAliveRequest;
using etcdserverpb::RangeRequest;
using etcdserverpb::RangeResponse;

etcdv3::AsyncLeaseKeepAliveAction::AsyncLeaseKeepAliveAction(etcdv3::ActionParameters param) : etcdv3::Action(param) {
  stream = parameters.lease_stub->AsyncLeaseKeepAlive(&context, &cq_, reinterpret_cast<void*>(Type::CONNECT));

  LeaseKeepAliveRequest request;
  request.set_id(parameters.lease_id);

  // wait "create" success (the stream becomes ready)
  void* got_tag;
  bool ok = false;
  if (cq_.Next(&got_tag, &ok) && ok && got_tag == reinterpret_cast<void*>(Type::CONNECT)) {
    stream->Write(request, reinterpret_cast<void*>(Type::WRITE));
  } else {
    throw std::runtime_error("failed to create a keepalive connection");
  }

  // wait "write" (LeaseKeepAliveRequest) success, and start to read the first
  // reply
  if (cq_.Next(&got_tag, &ok) && ok && got_tag == reinterpret_cast<void*>(Type::WRITE)) {
    stream->Read(&reply, (void*)this);
  } else {
    throw std::runtime_error("failed to write LeaseKeepAliveRequest to server");
  }
}

void etcdv3::AsyncLeaseKeepAliveAction::waitForResponse() {
  void* got_tag;
  bool ok = false;

  while (cq_.Next(&got_tag, &ok)) {
    if (ok == false) {
      break;
    }
    if (got_tag == reinterpret_cast<void*>(Type::WRITES_DONE)) {
      cq_.Shutdown();
      break;
    }
    if (got_tag == (void*)this)  // read tag
    {
      if (reply.ByteSize()) {
        stream->WritesDone(reinterpret_cast<void*>(Type::WRITES_DONE));
      } else {
        stream->Read(&reply, (void*)this);
      }
    }
  }
}

etcdv3::AsyncLeaseKeepAliveResponse etcdv3::AsyncLeaseKeepAliveAction::ParseResponse() {
  AsyncLeaseKeepAliveResponse resp;
  if (!status.ok()) {
    resp.set_error_code(status.error_code());
    resp.set_error_message(status.error_message());
  } else {
    resp.ParseResponse(reply);
  }
  return resp;
}
