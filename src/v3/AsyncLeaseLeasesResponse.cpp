#include "etcd/v3/AsyncLeaseLeasesResponse.hpp"
#include "etcd/v3/action_constants.hpp"

void etcdv3::AsyncLeaseLeasesResponse::ParseResponse(
    LeaseLeasesResponse &resp) {
  index = resp.header().revision();

  for (int index = 0; index < resp.leases_size(); index++) {
    leases.push_back(resp.leases(index).id());
  }
}
