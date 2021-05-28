#ifndef __ETCD_ACTION_CONSTANTS_HPP__
#define __ETCD_ACTION_CONSTANTS_HPP__

namespace etcdv3
{
  extern char const * CREATE_ACTION;
  extern char const * UPDATE_ACTION;
  extern char const * SET_ACTION;
  extern char const * GET_ACTION;
  extern char const * DELETE_ACTION;
  extern char const * COMPARESWAP_ACTION;
  extern char const * COMPAREDELETE_ACTION;
  extern char const * LOCK_ACTION;
  extern char const * UNLOCK_ACTION;
  extern char const * TXN_ACTION;

  extern char const * NUL;

  extern char const * KEEPALIVE_CREATE;
  extern char const * KEEPALIVE_WRITE;
  extern char const * KEEPALIVE_READ;
  extern char const * KEEPALIVE_DONE;

  extern char const * WATCH_CREATE;
  extern char const * WATCH_WRITE;
  extern char const * WATCH_WRITES_DONE;
}

#endif
