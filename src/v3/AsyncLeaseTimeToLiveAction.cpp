#include "etcd/v3/AsyncLeaseTimeToLiveAction.hpp"
#include "etcd/v3/Transaction.hpp"
#include "etcd/v3/action_constants.hpp"

using etcdserverpb::LeaseTimeToLiveRequest;

etcdv3::AsyncLeaseTimeToLiveAction::AsyncLeaseTimeToLiveAction(
    etcdv3::ActionParameters param)
    : etcdv3::Action(param) {
  etcdv3::Transaction transaction;
  transaction.setup_lease_timetolive_operation(parameters.lease_id,
                                               parameters.keys);

  response_reader = parameters.lease_stub->AsyncLeaseTimeToLive(
      &context, transaction.leasetimetolive_request, &cq_);
  response_reader->Finish(&reply, &status, (void *)this);
}

etcdv3::AsyncLeaseTimeToLiveResponse
etcdv3::AsyncLeaseTimeToLiveAction::ParseResponse() {
  AsyncLeaseTimeToLiveResponse lease_resp;
  if (!status.ok()) {
    lease_resp.set_error_code(status.error_code());
    lease_resp.set_error_message(status.error_message());
  } else {
    lease_resp.ParseResponse(reply);
  }
  return lease_resp;
}
