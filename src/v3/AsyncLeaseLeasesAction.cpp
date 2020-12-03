#include "etcd/v3/AsyncLeaseLeasesAction.hpp"

#include "etcd/v3/Transaction.hpp"
#include "etcd/v3/action_constants.hpp"


using etcdserverpb::LeaseLeasesRequest;

etcdv3::AsyncLeaseLeasesAction::AsyncLeaseLeasesAction(etcdv3::ActionParameters param) : etcdv3::Action(param) {
  etcdv3::Transaction transaction;
  transaction.setup_lease_leases_operation();

  response_reader = parameters.lease_stub->AsyncLeaseLeases(&context, transaction.leaseleases_request, &cq_);
  response_reader->Finish(&reply, &status, (void*)this);
}

etcdv3::AsyncLeaseLeasesResponse etcdv3::AsyncLeaseLeasesAction::ParseResponse() {
  AsyncLeaseLeasesResponse lease_resp;
  if (!status.ok()) {
    lease_resp.set_error_code(status.error_code());
    lease_resp.set_error_message(status.error_message());
  } else {
    lease_resp.ParseResponse(reply);
  }
  return lease_resp;
}
