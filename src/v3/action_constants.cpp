#include "etcd/v3/action_constants.hpp"

char const * etcdv3::CREATE_ACTION = "create";
char const * etcdv3::COMPARESWAP_ACTION = "compareAndSwap";
char const * etcdv3::UPDATE_ACTION = "update";
char const * etcdv3::SET_ACTION = "set";
char const * etcdv3::GET_ACTION = "get";
char const * etcdv3::DELETE_ACTION = "delete";
char const * etcdv3::COMPAREDELETE_ACTION = "compareAndDelete";
char const * etcdv3::LOCK_ACTION = "lock";
char const * etcdv3::UNLOCK_ACTION = "unlock";
char const * etcdv3::TXN_ACTION = "txn";

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
