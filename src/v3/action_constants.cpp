#include "etcd/v3/action_constants.hpp"

char const * etcdv3::CREATE_ACTION = "create";
char const * etcdv3::COMPARESWAP_ACTION = "compareAndSwap";
char const * etcdv3::UPDATE_ACTION = "update";
char const * etcdv3::SET_ACTION = "set";
char const * etcdv3::GET_ACTION = "get";
char const * etcdv3::PUT_ACTION = "put";
char const * etcdv3::DELETE_ACTION = "delete";
char const * etcdv3::COMPAREDELETE_ACTION = "compareAndDelete";
char const * etcdv3::LOCK_ACTION = "lock";
char const * etcdv3::UNLOCK_ACTION = "unlock";
char const * etcdv3::TXN_ACTION = "txn";
char const * etcdv3::WATCH_ACTION = "watch";

char const * etcdv3::LEASEGRANT = "leasegrant";
char const * etcdv3::LEASEREVOKE = "leaserevoke";
char const * etcdv3::LEASEKEEPALIVE = "leasekeepalive";
char const * etcdv3::LEASETIMETOLIVE = "leasetimetolive";
char const * etcdv3::LEASELEASES = "leaseleases";

char const * etcdv3::CAMPAIGN_ACTION = "campaign";
char const * etcdv3::PROCLAIM_ACTION = "preclaim";
char const * etcdv3::LEADER_ACTION = "leader";
char const * etcdv3::OBSERVE_ACTION = "obverse";
char const * etcdv3::RESIGN_ACTION = "resign";

// see: noPrefixEnd in etcd, however c++ doesn't allows '\0' inside a string, thus we use
// the UTF-8 char U+0000 (i.e., "\xC0\x80").
char const * etcdv3::NUL = "\xC0\x80";

char const * etcdv3::KEEPALIVE_CREATE = "keepalive create";
char const * etcdv3::KEEPALIVE_WRITE = "keepalive write";
char const * etcdv3::KEEPALIVE_READ = "keepalive read";
char const * etcdv3::KEEPALIVE_DONE = "keepalive done";

char const * etcdv3::WATCH_CREATE = "watch create";
char const * etcdv3::WATCH_WRITE = "watch write";
char const * etcdv3::WATCH_WRITES_DONE = "watch writes done";

char const * etcdv3::ELECTION_OBSERVE_CREATE = "observe create";

const int etcdv3::ERROR_KEY_NOT_FOUND = 100;
const int etcdv3::ERROR_COMPARE_FAILED = 101;
const int etcdv3::ERROR_KEY_ALREADY_EXISTS = 105;
