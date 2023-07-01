#ifndef __ETCD_ACTION_CONSTANTS_HPP__
#define __ETCD_ACTION_CONSTANTS_HPP__

#include <string>

namespace etcdv3 {
extern char const* CREATE_ACTION;
extern char const* UPDATE_ACTION;
extern char const* SET_ACTION;
extern char const* GET_ACTION;
extern char const* PUT_ACTION;
extern char const* DELETE_ACTION;
extern char const* COMPARESWAP_ACTION;
extern char const* COMPAREDELETE_ACTION;
extern char const* LOCK_ACTION;
extern char const* UNLOCK_ACTION;
extern char const* TXN_ACTION;
extern char const* WATCH_ACTION;

extern char const* LEASEGRANT;
extern char const* LEASEREVOKE;
extern char const* LEASEKEEPALIVE;
extern char const* LEASETIMETOLIVE;
extern char const* LEASELEASES;

extern char const* CAMPAIGN_ACTION;
extern char const* PROCLAIM_ACTION;
extern char const* LEADER_ACTION;
extern char const* OBSERVE_ACTION;
extern char const* RESIGN_ACTION;

extern std::string const NUL;

extern char const* KEEPALIVE_CREATE;
extern char const* KEEPALIVE_WRITE;
extern char const* KEEPALIVE_READ;
extern char const* KEEPALIVE_DONE;
extern char const* KEEPALIVE_FINISH;

extern char const* WATCH_CREATE;
extern char const* WATCH_WRITE;
extern char const* WATCH_WRITE_CANCEL;
extern char const* WATCH_WRITES_DONE;
extern char const* WATCH_FINISH;

extern char const* ELECTION_OBSERVE_CREATE;
extern char const* ELECTION_OBSERVE_FINISH;

extern const int ERROR_GRPC_OK;
extern const int ERROR_GRPC_CANCELLED;
extern const int ERROR_GRPC_UNKNOWN;
extern const int ERROR_GRPC_INVALID_ARGUMENT;
extern const int ERROR_GRPC_DEADLINE_EXCEEDED;
extern const int ERROR_GRPC_NOT_FOUND;
extern const int ERROR_GRPC_ALREADY_EXISTS;
extern const int ERROR_GRPC_PERMISSION_DENIED;
extern const int ERROR_GRPC_UNAUTHENTICATED;
extern const int ERROR_GRPC_RESOURCE_EXHAUSTED;
extern const int ERROR_GRPC_FAILED_PRECONDITION;
extern const int ERROR_GRPC_ABORTED;
extern const int ERROR_GRPC_OUT_OF_RANGE;
extern const int ERROR_GRPC_UNIMPLEMENTED;
extern const int ERROR_GRPC_INTERNAL;
extern const int ERROR_GRPC_UNAVAILABLE;
extern const int ERROR_GRPC_DATA_LOSS;

extern const int ERROR_KEY_NOT_FOUND;
extern const int ERROR_COMPARE_FAILED;
extern const int ERROR_KEY_ALREADY_EXISTS;
extern const int ERROR_ACTION_CANCELLED;
}  // namespace etcdv3

#endif
