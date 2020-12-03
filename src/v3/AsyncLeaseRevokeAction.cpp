#include "etcd/v3/AsyncLeaseRevokeAction.hpp"
#include "etcd/v3/Transaction.hpp"
#include "etcd/v3/action_constants.hpp"

using etcdserverpb::LeaseRevokeRequest;

etcdv3::AsyncLeaseRevokeAction::AsyncLeaseRevokeAction(
    etcdv3::ActionParameters param)
    : etcdv3::Action(param) {
  etcdv3::Transaction transaction;
  transaction.setup_lease_revoke_operation(parameters.lease_id);

  response_reader = parameters.lease_stub->AsyncLeaseRevoke(
      &context, transaction.leaserevoke_request, &cq_);
  response_reader->Finish(&reply, &status, (void *)this);
}

etcdv3::AsyncLeaseRevokeResponse
etcdv3::AsyncLeaseRevokeAction::ParseResponse() {
  AsyncLeaseRevokeResponse lease_resp;
  if (!status.ok()) {
    lease_resp.set_error_code(status.error_code());
    lease_resp.set_error_message(status.error_message());
  } else {
    lease_resp.ParseResponse(reply);
  }
  return lease_resp;
}
