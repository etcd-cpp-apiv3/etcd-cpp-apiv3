#include "etcd/v3/Action.hpp"
#include <grpc/support/log.h>
#include <grpcpp/support/status.h>
#include "etcd/v3/action_constants.hpp"
#include <cstdlib>

#ifndef GPR_ASSERT
#define GPR_ASSERT(x)                                             \
  if (!(x)) {                                                     \
    fprintf(stderr, "%s:%d assert failed\n", __FILE__, __LINE__); \
    abort();                                                      \
}
#endif

etcdv3::Action::Action(etcdv3::ActionParameters const& params) {
  parameters = params;
  this->InitAction();
}

etcdv3::Action::Action(etcdv3::ActionParameters&& params) {
  parameters = std::move(params);
  this->InitAction();
}

etcdv3::Action::~Action() {
  cq_.Shutdown();

  // cancel on-the-fly calls
  context.TryCancel();
}

void etcdv3::Action::InitAction() {
  if (!parameters.auth_token.empty()) {
    // use `token` as the key, see:
    //
    //  etcd/etcdserver/api/v3rpc/rpctypes/metadatafields.go
    context.AddMetadata("token", parameters.auth_token);
  }
  start_timepoint = std::chrono::high_resolution_clock::now();
}

etcdv3::ActionParameters::ActionParameters() {
  withPrefix = false;
  revision = 0;
  old_revision = 0;
  lease_id = 0;
  ttl = 0;
  keys_only = false;
  count_only = false;
  kv_stub = NULL;
  watch_stub = NULL;
  lease_stub = NULL;
}

bool etcdv3::ActionParameters::has_grpc_timeout() const {
  return this->grpc_timeout != std::chrono::microseconds::zero();
}

std::chrono::system_clock::time_point etcdv3::ActionParameters::grpc_deadline()
    const {
  return std::chrono::system_clock::now() + this->grpc_timeout;
}

void etcdv3::ActionParameters::dump(std::ostream& os) const {
  os << "ActionParameters:" << std::endl;
  os << "  withPrefix:    " << withPrefix << std::endl;
  os << "  revision:      " << revision << std::endl;
  os << "  old_revision:  " << old_revision << std::endl;
  os << "  lease_id:      " << lease_id << std::endl;
  os << "  ttl:           " << ttl << std::endl;
  os << "  limit:         " << limit << std::endl;
  os << "  name:          " << name << std::endl;
  os << "  key:           " << key << std::endl;
  os << "  range_end:     " << range_end << std::endl;
  os << "  keys_only:     " << keys_only << std::endl;
  os << "  count_only:    " << count_only << std::endl;
  os << "  value:         " << value << std::endl;
  os << "  old_value:     " << old_value << std::endl;
  os << "  auth_token:    " << auth_token << std::endl;
  os << "  grpc_timeout:  " << grpc_timeout.count() << "(ms)" << std::endl;
}

void etcdv3::Action::waitForResponse() {
  void* got_tag;
  bool ok = false;

  if (parameters.has_grpc_timeout()) {
    switch (cq_.AsyncNext(&got_tag, &ok, parameters.grpc_deadline())) {
    case CompletionQueue::NextStatus::TIMEOUT: {
      status =
          grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED, "gRPC timeout");
      break;
    }
    case CompletionQueue::NextStatus::SHUTDOWN: {
      status =
          grpc::Status(grpc::StatusCode::UNAVAILABLE, "gRPC already shutdown");
      break;
    }
    case CompletionQueue::NextStatus::GOT_EVENT: {
      if (!ok) {
        status =
            grpc::Status(grpc::StatusCode::ABORTED,
                         "Failed to execute the action: not ok or invalid tag");
      }
      break;
    }
    }
  } else {
    cq_.Next(&got_tag, &ok);
    GPR_ASSERT(got_tag == (void*) this);
  }
}

const std::chrono::high_resolution_clock::time_point
etcdv3::Action::startTimepoint() {
  return this->start_timepoint;
}

std::string etcdv3::detail::string_plus_one(std::string const& value) {
  // Referred from the Go implementation in etcd.
  for (int32_t i = value.size() - 1; i >= 0; --i) {
    if (static_cast<unsigned char>(value[i]) < 0xff) {
      std::string s = value.substr(0, i + 1);
      s[i] = s[i] + 1;
      return s;
    }
  }
  return {etcdv3::NUL};
}

std::string etcdv3::detail::resolve_etcd_endpoints(
    std::string const& default_endpoints) {
  const char* ep = std::getenv("ETCD_ENDPOINTS");
  return ep ? ep : default_endpoints;
}
