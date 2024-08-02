#include "etcd/v3/action_constants.hpp"

char const* etcdv3::CREATE_ACTION = "create";
char const* etcdv3::COMPARESWAP_ACTION = "compareAndSwap";
char const* etcdv3::UPDATE_ACTION = "update";
char const* etcdv3::SET_ACTION = "set";
char const* etcdv3::GET_ACTION = "get";
char const* etcdv3::PUT_ACTION = "set";  // alias
char const* etcdv3::DELETE_ACTION = "delete";
char const* etcdv3::COMPAREDELETE_ACTION = "compareAndDelete";
char const* etcdv3::LOCK_ACTION = "lock";
char const* etcdv3::UNLOCK_ACTION = "unlock";
char const* etcdv3::TXN_ACTION = "txn";
char const* etcdv3::WATCH_ACTION = "watch";

char const* etcdv3::LEASEGRANT = "leasegrant";
char const* etcdv3::LEASEREVOKE = "leaserevoke";
char const* etcdv3::LEASEKEEPALIVE = "leasekeepalive";
char const* etcdv3::LEASETIMETOLIVE = "leasetimetolive";
char const* etcdv3::LEASELEASES = "leaseleases";

char const* etcdv3::ADDMEMBER = "addmember";
char const* etcdv3::LISTMEMBER = "listmember";
char const* etcdv3::REMOVEMEMBER = "removemember";

char const* etcdv3::CAMPAIGN_ACTION = "campaign";
char const* etcdv3::PROCLAIM_ACTION = "preclaim";
char const* etcdv3::LEADER_ACTION = "leader";
char const* etcdv3::OBSERVE_ACTION = "obverse";
char const* etcdv3::RESIGN_ACTION = "resign";

// see: noPrefixEnd in etcd, however c++ doesn't allows naive '\0' inside
// a string, thus we use std::string(1, '\x00') as the constructor.
std::string const etcdv3::NUL = std::string(1, '\x00');

char const* etcdv3::KEEPALIVE_CREATE = "keepalive create";
char const* etcdv3::KEEPALIVE_WRITE = "keepalive write";
char const* etcdv3::KEEPALIVE_READ = "keepalive read";
char const* etcdv3::KEEPALIVE_DONE = "keepalive done";
char const* etcdv3::KEEPALIVE_FINISH = "keepalive finish";

char const* etcdv3::WATCH_CREATE = "watch create";
char const* etcdv3::WATCH_WRITE = "watch write";
char const* etcdv3::WATCH_WRITE_CANCEL = "watch write cancel";
char const* etcdv3::WATCH_WRITES_DONE = "watch writes done";
char const* etcdv3::WATCH_FINISH = "watch finish";

char const* etcdv3::ELECTION_OBSERVE_CREATE = "observe create";
char const* etcdv3::ELECTION_OBSERVE_FINISH = "observe finish";

const int etcdv3::ERROR_GRPC_OK = 0;
const int etcdv3::ERROR_GRPC_CANCELLED = 1;
const int etcdv3::ERROR_GRPC_UNKNOWN = 2;
const int etcdv3::ERROR_GRPC_INVALID_ARGUMENT = 3;
const int etcdv3::ERROR_GRPC_DEADLINE_EXCEEDED = 4;
const int etcdv3::ERROR_GRPC_NOT_FOUND = 5;
const int etcdv3::ERROR_GRPC_ALREADY_EXISTS = 6;
const int etcdv3::ERROR_GRPC_PERMISSION_DENIED = 7;
const int etcdv3::ERROR_GRPC_UNAUTHENTICATED = 16;
const int etcdv3::ERROR_GRPC_RESOURCE_EXHAUSTED = 8;
const int etcdv3::ERROR_GRPC_FAILED_PRECONDITION = 9;
const int etcdv3::ERROR_GRPC_ABORTED = 10;
const int etcdv3::ERROR_GRPC_OUT_OF_RANGE = 11;
const int etcdv3::ERROR_GRPC_UNIMPLEMENTED = 12;
const int etcdv3::ERROR_GRPC_INTERNAL = 13;
const int etcdv3::ERROR_GRPC_UNAVAILABLE = 14;
const int etcdv3::ERROR_GRPC_DATA_LOSS = 15;

const int etcdv3::ERROR_KEY_NOT_FOUND = 100;
const int etcdv3::ERROR_COMPARE_FAILED = 101;
const int etcdv3::ERROR_KEY_ALREADY_EXISTS = 105;
const int etcdv3::ERROR_ACTION_CANCELLED = 106;
