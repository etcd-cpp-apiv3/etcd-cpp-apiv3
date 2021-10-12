#ifndef __ETCD_ACTION_CONSTANTS_HPP__
#define __ETCD_ACTION_CONSTANTS_HPP__

namespace etcdv3
{
  extern char const * CREATE_ACTION;
  extern char const * UPDATE_ACTION;
  extern char const * SET_ACTION;
  extern char const * GET_ACTION;
  extern char const * PUT_ACTION;
  extern char const * DELETE_ACTION;
  extern char const * COMPARESWAP_ACTION;
  extern char const * COMPAREDELETE_ACTION;
  extern char const * LOCK_ACTION;
  extern char const * UNLOCK_ACTION;
  extern char const * TXN_ACTION;
  extern char const * WATCH_ACTION;

  extern char const * LEASEGRANT;
  extern char const * LEASEREVOKE;
  extern char const * LEASEKEEPALIVE;
  extern char const * LEASETIMETOLIVE;
  extern char const * LEASELEASES;

  extern char const * CAMPAIGN_ACTION;
  extern char const * PROCLAIM_ACTION;
  extern char const * LEADER_ACTION;
  extern char const * OBSERVE_ACTION;
  extern char const * RESIGN_ACTION;

  extern char const * NUL;

  extern char const * KEEPALIVE_CREATE;
  extern char const * KEEPALIVE_WRITE;
  extern char const * KEEPALIVE_READ;
  extern char const * KEEPALIVE_DONE;

  extern char const * WATCH_CREATE;
  extern char const * WATCH_WRITE;
  extern char const * WATCH_WRITES_DONE;

  extern char const * ELECTION_OBSERVE_CREATE;

  extern const int ERROR_KEY_NOT_FOUND;
  extern const int ERROR_COMPARE_FAILED;
  extern const int ERROR_KEY_ALREADY_EXISTS;
}

#endif
