#ifndef __ETCD_CLIENT_HPP__
#define __ETCD_CLIENT_HPP__

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "pplx/pplxtasks.h"

#include "etcd/Response.hpp"
#include "etcd/v3/action_constants.hpp"

namespace etcdv3 {
  class Transaction;
  class AsyncObserveAction;

  namespace detail {
    std::string string_plus_one(std::string const &value);
  }
}

#if defined(WITH_GRPC_CHANNEL_CLASS)
namespace grpc {
  class Channel;
  class ChannelArguments;
}
#else
namespace grpc_impl {
  class Channel;
  class ChannelArguments;
}
#endif

namespace etcd
{
  using etcdv3::ERROR_KEY_NOT_FOUND;
  using etcdv3::ERROR_COMPARE_FAILED;
  using etcdv3::ERROR_KEY_ALREADY_EXISTS;

  class KeepAlive;
  class Watcher;

  /**
   * Client is responsible for maintaining a connection towards an etcd server.
   * Etcd operations can be reached via the methods of the client.
   */
  class Client
  {
  private:
    class TokenAuthenticator;
    class TokenAuthenticatorDeleter {
      public:
        void operator()(TokenAuthenticator *authenticator);
    };

  public:
    /**
     * Constructs an etcd client object.
     *
     * @param etcd_url is the url of the etcd server to connect to, like "http://127.0.0.1:2379",
     *                 or multiple url, seperated by ',' or ';'.
     * @param load_balancer is the load balance strategy, can be one of round_robin/pick_first/grpclb/xds.
     */
    Client(std::string const & etcd_url,
           std::string const & load_balancer = "round_robin");

    /**
     * Constructs an etcd client object.
     *
     * @param etcd_url is the url of the etcd server to connect to, like "http://127.0.0.1:2379",
     *                 or multiple url, seperated by ',' or ';'.
     * @param arguments user provided grpc channel arguments.
     */
    Client(std::string const & etcd_url,
#if defined(WITH_GRPC_CHANNEL_CLASS)
           grpc::ChannelArguments const & arguments
#else
           grpc_impl::ChannelArguments const & arguments
#endif
           );

    /**
     * Constructs an etcd client object.
     *
     * @param etcd_url is the url of the etcd server to connect to, like "http://127.0.0.1:2379",
     *                 or multiple url, seperated by ',' or ';'.
     * @param load_balancer is the load balance strategy, can be one of round_robin/pick_first/grpclb/xds.
     */
    static etcd::Client *WithUrl(std::string const & etcd_url,
                                 std::string const & load_balancer = "round_robin");

    /**
     * Constructs an etcd client object.
     *
     * @param etcd_url is the url of the etcd server to connect to, like "http://127.0.0.1:2379",
     *                 or multiple url, seperated by ',' or ';'.
     * @param arguments user provided grpc channel arguments.
     */
    static etcd::Client *WithUrl(std::string const & etcd_url,
#if defined(WITH_GRPC_CHANNEL_CLASS)
                                 grpc::ChannelArguments const & arguments
#else
                                 grpc_impl::ChannelArguments const & arguments
#endif
                                 );

    /**
     * Constructs an etcd client object.
     *
     * @param etcd_url is the url of the etcd server to connect to, like "http://127.0.0.1:2379",
     *                 or multiple url, seperated by ',' or ';'.
     * @param username username of etcd auth
     * @param password password of etcd auth
     * @param load_balancer is the load balance strategy, can be one of round_robin/pick_first/grpclb/xds.
     * @param auth_token_ttl TTL seconds for auth token, see also `--auth-token-ttl` flags of etcd.
     */
    Client(std::string const & etcd_url,
           std::string const & username,
           std::string const & password,
           int const auth_token_ttl = 300,
           std::string const & load_balancer = "round_robin");

    /**
     * Constructs an etcd client object.
     *
     * @param etcd_url is the url of the etcd server to connect to, like "http://127.0.0.1:2379",
     *                 or multiple url, seperated by ',' or ';'.
     * @param username username of etcd auth
     * @param password password of etcd auth
     * @param arguments user provided grpc channel arguments.
     * @param auth_token_ttl TTL seconds for auth token, see also `--auth-token-ttl` flags of etcd.
     *                       Default value should be 300.
     */
    Client(std::string const & etcd_url,
           std::string const & username,
           std::string const & password,
           int const auth_token_ttl,
#if defined(WITH_GRPC_CHANNEL_CLASS)
           grpc::ChannelArguments const & arguments
#else
           grpc_impl::ChannelArguments const & arguments
#endif
           );


    /**
     * Constructs an etcd client object.
     *
     * @param etcd_url is the url of the etcd server to connect to, like "http://127.0.0.1:2379",
     *                 or multiple url, seperated by ',' or ';'.
     * @param username username of etcd auth
     * @param password password of etcd auth
     * @param load_balancer is the load balance strategy, can be one of round_robin/pick_first/grpclb/xds.
     * @param auth_token_ttl TTL seconds for auth token, see also `--auth-token-ttl` flags of etcd.
     */
    static etcd::Client *WithUser(std::string const & etcd_url,
                                  std::string const & username,
                                  std::string const & password,
                                  int const auth_token_ttl = 300,
                                  std::string const & load_balancer = "round_robin");

    /**
     * Constructs an etcd client object.
     *
     * @param etcd_url is the url of the etcd server to connect to, like "http://127.0.0.1:2379",
     *                 or multiple url, seperated by ',' or ';'.
     * @param username username of etcd auth
     * @param password password of etcd auth
     * @param arguments user provided grpc channel arguments.
     * @param auth_token_ttl TTL seconds for auth token, see also `--auth-token-ttl` flags of etcd.
     *                       Default value should be 300.
     */
    static etcd::Client *WithUser(std::string const & etcd_url,
                                  std::string const & username,
                                  std::string const & password,
                                  int const auth_token_ttl,
#if defined(WITH_GRPC_CHANNEL_CLASS)
                                  grpc::ChannelArguments const & arguments
#else
                                  grpc_impl::ChannelArguments const & arguments
#endif
                                  );


    /**
     * Constructs an etcd client object.
     *
     * @param etcd_url is the url of the etcd server to connect to, like "http://127.0.0.1:2379",
     *                 or multiple url, seperated by ',' or ';'.
     * @param ca   root CA file for SSL/TLS connection.
     * @param cert cert chain file for SSL/TLS authentication, could be empty string.
     * @param key  private key file for SSL/TLS authentication, could be empty string.
     * @param load_balancer is the load balance strategy, can be one of round_robin/pick_first/grpclb/xds.
     */
    Client(std::string const & etcd_url,
           std::string const & ca,
           std::string const & cert,
           std::string const & key,
           std::string const & target_name_override,
           std::string const & load_balancer);

    /**
     * Constructs an etcd client object.
     *
     * @param etcd_url is the url of the etcd server to connect to, like "http://127.0.0.1:2379",
     *                 or multiple url, seperated by ',' or ';'.
     * @param ca   root CA file for SSL/TLS connection.
     * @param cert cert chain file for SSL/TLS authentication, could be empty string.
     * @param key  private key file for SSL/TLS authentication, could be empty string.
     * @param arguments user provided grpc channel arguments.
     */
    Client(std::string const & etcd_url,
           std::string const & ca,
           std::string const & cert,
           std::string const & key,
           std::string const & target_name_override,
#if defined(WITH_GRPC_CHANNEL_CLASS)
           grpc::ChannelArguments const & arguments
#else
           grpc_impl::ChannelArguments const & arguments
#endif
           );


    /**
     * Constructs an etcd client object.
     *
     * @param etcd_url is the url of the etcd server to connect to, like "http://127.0.0.1:2379",
     *                 or multiple url, seperated by ',' or ';'.
     * @param ca   root CA file for SSL/TLS connection.
     * @param cert cert chain file for SSL/TLS authentication, could be empty string.
     * @param key  private key file for SSL/TLS authentication, could be empty string.
     * @param target_name_override Override the target host name if you want to pass multiple address
     * for load balancing with SSL, and there's no DNS. The @target_name_override@ must exist in the
     * SANS of your SSL certificate.
     * @param load_balancer is the load balance strategy, can be one of round_robin/pick_first/grpclb/xds.
     */
    static etcd::Client *WithSSL(std::string const & etcd_url,
                                 std::string const & ca,
                                 std::string const & cert = "",
                                 std::string const & key = "",
                                 std::string const & target_name_override = "",
                                 std::string const & load_balancer = "round_robin");

    /**
     * Constructs an etcd client object.
     *
     * @param etcd_url is the url of the etcd server to connect to, like "http://127.0.0.1:2379",
     *                 or multiple url, seperated by ',' or ';'.
     * @param ca   root CA file for SSL/TLS connection.
     * @param cert cert chain file for SSL/TLS authentication, could be empty string.
     * @param key  private key file for SSL/TLS authentication, could be empty string.
     * @param target_name_override Override the target host name if you want to pass multiple address
     * for load balancing with SSL, and there's no DNS. The @target_name_override@ must exist in the
     * SANS of your SSL certificate.
     * @param arguments user provided grpc channel arguments.
     */
    static etcd::Client *WithSSL(std::string const & etcd_url,
#if defined(WITH_GRPC_CHANNEL_CLASS)
                                 grpc::ChannelArguments const & arguments,
#else
                                 grpc_impl::ChannelArguments const & arguments,
#endif
                                 std::string const & ca,
                                 std::string const & cert = "",
                                 std::string const & key = "",
                                 std::string const & target_name_override = "");

    /**
     * Get the HEAD revision of the connected etcd server.
     */
    pplx::task<Response> head();

    /**
     * Get the value of specified key from the etcd server
     * @param key is the key to be read
     */
    pplx::task<Response> get(std::string const & key);

    /**
     * Sets the value of a key. The key will be modified if already exists or created
     * if it does not exists.
     * @param key is the key to be created or modified
     * @param value is the new value to be set
     */
    pplx::task<Response> set(std::string const & key, std::string const & value, int ttl = 0);

    /**
     * Sets the value of a key. The key will be modified if already exists or created
     * if it does not exists.
     * @param key is the key to be created or modified
     * @param value is the new value to be set
     * @param leaseId is the lease attached to the key
     */
    pplx::task<Response> set(std::string const & key, std::string const & value, int64_t leaseId);


    /**
     * Creates a new key and sets it's value. Fails if the key already exists.
     * @param key is the key to be created
     * @param value is the value to be set
     */
    pplx::task<Response> add(std::string const & key, std::string const & value, int ttl = 0);

    /**
     * Creates a new key and sets it's value. Fails if the key already exists.
     * @param key is the key to be created
     * @param value is the value to be set
     * @param leaseId is the lease attached to the key
     */
    pplx::task<Response> add(std::string const & key, std::string const & value, int64_t leaseId);

    /**
     * Put a new key-value pair.
     * @param key is the key to be put
     * @param value is the value to be put
     */
    pplx::task<Response> put(std::string const & key, std::string const & value);

    /**
     * Modifies an existing key. Fails if the key does not exists.
     * @param key is the key to be modified
     * @param value is the new value to be set
     */
    pplx::task<Response> modify(std::string const & key, std::string const & value, int ttl = 0);

    /**
     * Modifies an existing key. Fails if the key does not exists.
     * @param key is the key to be modified
     * @param value is the new value to be set
     * @param leaseId is the lease attached to the key
     */
    pplx::task<Response> modify(std::string const & key, std::string const & value, int64_t leaseId);

    /**
     * Modifies an existing key only if it has a specific value. Fails if the key does not exists
     * or the original value differs from the expected one.
     * @param key is the key to be modified
     * @param value is the new value to be set
     * @param old_value is the value to be replaced
     */
    pplx::task<Response> modify_if(std::string const & key, std::string const & value, std::string const & old_value, int ttl = 0);

    /**
     * Modifies an existing key only if it has a specific value. Fails if the key does not exists
     * or the original value differs from the expected one.
     * @param key is the key to be modified
     * @param value is the new value to be set
     * @param old_value is the value to be replaced
     * @param leaseId is the lease attached to the key
     */
    pplx::task<Response> modify_if(std::string const & key, std::string const & value, std::string const & old_value, int64_t leaseId);

    /**
     * Modifies an existing key only if it has a specific modification index value. Fails if the key
     * does not exists or the modification index of the previous value differs from the expected one.
     * @param key is the key to be modified
     * @param value is the new value to be set
     * @param old_index is the expected index of the original value
     */
    pplx::task<Response> modify_if(std::string const & key, std::string const & value, int64_t old_index, int ttl = 0);

    /**
     * Modifies an existing key only if it has a specific modification index value. Fails if the key
     * does not exists or the modification index of the previous value differs from the expected one.
     * @param key is the key to be modified
     * @param value is the new value to be set
     * @param old_index is the expected index of the original value
     * @param leaseId is the lease attached to the key
     */
    pplx::task<Response> modify_if(std::string const & key, std::string const & value, int64_t old_index, int64_t leaseId);

    /**
     * Removes a single key. The key has to point to a plain, non directory entry.
     * @param key is the key to be deleted
     */
    pplx::task<Response> rm(std::string const & key);

    /**
     * Removes a single key but only if it has a specific value. Fails if the key does not exists
     * or the its value differs from the expected one.
     * @param key is the key to be deleted
     */
    pplx::task<Response> rm_if(std::string const & key, std::string const & old_value);

    /**
     * Removes an existing key only if it has a specific modification index value. Fails if the key
     * does not exists or the modification index of it differs from the expected one.
     * @param key is the key to be deleted
     * @param old_index is the expected index of the existing value
     */
    pplx::task<Response> rm_if(std::string const & key, int64_t old_index);

    /**
     * Gets a directory listing of the directory identified by the key.
     * @param key is the key to be listed
     */
    pplx::task<Response> ls(std::string const & key);


    /**
     * Gets a directory listing of the directory identified by the key.
     * @param key is the key to be listed
     * @param limit is the size limit of results to be listed, we don't use default parameters
     *        to ensure backwards binary compatibility.
     */
    pplx::task<Response> ls(std::string const & key, size_t const limit);

    /**
     * Gets a directory listing of the directory identified by the key and range_end, i.e., get
     * all keys in the range [key, range_end).
     *
     * @param key is the key to be listed
     * @param range_end is the end of key range to be listed
     */
    pplx::task<Response> ls(std::string const & key, std::string const &range_end);


    /**
     * Gets a directory listing of the directory identified by the key and range_end, i.e., get
     * all keys in the range [key, range_end).
     *
     * @param key is the key to be listed
     * @param range_end is the end of key range to be listed
     * @param limit is the size limit of results to be listed, we don't use default parameters
     *        to ensure backwards binary compatibility.
     */
    pplx::task<Response> ls(std::string const & key, std::string const &range_end, size_t const limit);

    /**
     * Removes a directory node. Fails if the parent directory dos not exists or not a directory.
     * @param key is the directory to be created to be listed
     * @param recursive if true then delete a whole subtree, otherwise deletes only an empty directory.
     */
    pplx::task<Response> rmdir(std::string const & key, bool recursive = false);

    /**
     * Removes multiple keys between [key, range_end).
     *
     * This overload for `const char *` is to avoid const char * to bool implicit casting.
     *
     * @param key is the directory to be created to be listed
     * @param range_end is the end of key range to be removed.
     */
    pplx::task<Response> rmdir(std::string const & key, const char *range_end);

    /**
     * Removes multiple keys between [key, range_end).
     *
     * @param key is the directory to be created to be listed
     * @param range_end is the end of key range to be removed.
     */
    pplx::task<Response> rmdir(std::string const & key, std::string const &range_end);

    /**
     * Watches for changes of a key or a subtree. Please note that if you watch e.g. "/testdir" and
     * a new key is created, like "/testdir/newkey" then no change happened in the value of
     * "/testdir" so your watch will not detect this. If you want to detect addition and deletion of
     * directory entries then you have to do a recursive watch.
     * @param key is the value or directory to be watched
     * @param recursive if true watch a whole subtree
     */
    pplx::task<Response> watch(std::string const & key, bool recursive = false);

    /**
     * Watches for changes of a key or a subtree from a specific index. The index value can be in the "past".
     * @param key is the value or directory to be watched
     * @param fromIndex the first index we are interested in
     * @param recursive if true watch a whole subtree
     */
    pplx::task<Response> watch(std::string const & key, int64_t fromIndex, bool recursive = false);

    /**
     * Watches for changes of a range of keys inside [key, range_end).
     *
     * This overload for `const char *` is to avoid const char * to bool implicit casting.
     *
     * @param key is the value or directory to be watched
     * @param range_end is the end of key range to be removed.
     */
    pplx::task<Response> watch(std::string const & key, const char *range_end);

    /**
     * Watches for changes of a range of keys inside [key, range_end).
     *
     * @param key is the value or directory to be watched
     * @param range_end is the end of key range to be removed.
     */
    pplx::task<Response> watch(std::string const & key, std::string const &range_end);

    /**
     * Watches for changes of a range of keys inside [key, range_end) from a specific index. The index value
     * can be in the "past".
     *
     * Watches for changes of a key or a subtree from a specific index. The index value can be in the "past".
     * @param key is the value or directory to be watched
     * @param range_end is the end of key range to be removed.
     * @param fromIndex the first index we are interested in
     */
    pplx::task<Response> watch(std::string const & key, std::string const &range_end, int64_t fromIndex);

    /**
     * Grants a lease.
     * @param ttl is the time to live of the lease
     */
    pplx::task<Response> leasegrant(int ttl);

    /**
     * Grants a lease.
     * @param ttl is the time to live of the lease
     */
    pplx::task<std::shared_ptr<KeepAlive>> leasekeepalive(int ttl);

    /**
     * Revoke a lease.
     * @param lease_id is the id the lease
     */
    pplx::task<Response> leaserevoke(int64_t lease_id);

    /**
     * Get time-to-live of a lease.
     * @param lease_id is the id the lease
     */
    pplx::task<Response> leasetimetolive(int64_t lease_id);

    /**
     * Gains a lock at a key, using a default created lease, using the default lease (10 seconds), with
     * keeping alive has already been taken care of by the library.
     * @param key is the key to be used to request the lock.
     */
    pplx::task<Response> lock(std::string const &key);

    /**
     * Gains a lock at a key, using a default created lease, using the specified lease TTL (in seconds), with
     * keeping alive has already been taken care of by the library.
     * @param key is the key to be used to request the lock.
     * @param lease_ttl is the TTL used to create a lease for the key.
     */
    pplx::task<Response> lock(std::string const &key, int lease_ttl);

    /**
     * Gains a lock at a key, using a user-provided lease, the lifetime of the lease won't be taken care
     * of by the library.
     * @param key is the key to be used to request the lock.
     */
    pplx::task<Response> lock_with_lease(std::string const &key, int64_t lease_id);

    /**
     * Releases a lock at a key.
     * @param key is the lock key to release.
     */
    pplx::task<Response> unlock(std::string const &lock_key);

     /**
      * Execute a etcd transaction.
      * @param txn is the transaction object to be executed.
      */
    pplx::task<Response> txn(etcdv3::Transaction const &txn);

    /**
     * Campaign for the election @name@.
     *
     * @param name is the name of election that will campaign for.
     * @param lease_id is a user-managed (usually with a `KeepAlive`) lease id.
     * @param value is the value for campaign.
     *
     * @returns a leader key if succeed, consist of
     *
     *    - name: the name of the election
     *    - key:  a generated election key
     *    - created rev:  the revision of the generated key
     *    - lease: the lease id of the election leader
     */
    pplx::task<Response> campaign(std::string const &name, int64_t lease_id,
                                  std::string const &value);

    /**
     * Updates the value of election with a new value, with leader key returns by
     * @campaign@.
     *
     * @param name is the name of election
     * @param lease_id is the user-provided lease id for the proclamation
     * @param key is the generated associated key returned by @campaign@
     * @param revision is the created revision of key-value returned by @campaign@
     * @param value is the new value to set.
     */
    pplx::task<Response> proclaim(std::string const &name, int64_t lease_id,
                                  std::string const &key, int64_t revision, std::string const &value);

    /**
     * Get the current leader proclamation.
     *
     * @param name is the names of election.
     *
     * @returns current election key and value.
     */
    pplx::task<Response> leader(std::string const &name);

    /**
     * An observer that will cancel the associated election::observe request
     * when being destruct.
     */
    class Observer {
      public:
        ~Observer();
      private:
        std::shared_ptr<etcdv3::AsyncObserveAction> action = nullptr;
        pplx::task<etcd::Response> resp;

        friend class Client;
    };

    /**
     * Observe the leader change.
     *
     * @param name is the names of election to watch.
     *
     * @returns an observer that holds that action and will cancel the request when being destructed.
     */
    std::unique_ptr<Observer> observe(std::string const &name,
                                      const bool once = false);

    /**
     * Observe the leader change.
     *
     * @param name is the names of election to watch.
     *
     * @returns an observer that holds that action and will cancel the request when being destructed.
     */
    std::unique_ptr<Observer> observe(std::string const &name,
                                      std::function<void(Response)> callback,
                                      const bool once = false);

    /**
     * Updates the value of election with a new value, with leader key returns by
     * @campaign@.
     *
     * @param name is the name of election
     * @param lease_id is the user-provided lease id for the proclamation
     * @param key is the generated associated key returned by @campaign@
     * @param revision is the created revision of key-value returned by @campaign@
     */
    pplx::task<Response> resign(std::string const &name, int64_t lease_id,
                                std::string const &key, int64_t revision);

    /**
     * Return current auth token.
     */
    const std::string &current_auth_token() const;

  private:
#if defined(WITH_GRPC_CHANNEL_CLASS)
    std::shared_ptr<grpc::Channel> channel;
#else
    std::shared_ptr<grpc_impl::Channel> channel;
#endif

    mutable std::unique_ptr<TokenAuthenticator, TokenAuthenticatorDeleter> token_authenticator;

    struct EtcdServerStubs;
    struct EtcdServerStubsDeleter {
      void operator()(EtcdServerStubs *stubs);
    };
    std::unique_ptr<EtcdServerStubs, EtcdServerStubsDeleter> stubs;

    std::mutex mutex_for_keepalives;
    std::map<std::string, int64_t> leases_for_locks;
    std::map<int64_t, std::shared_ptr<KeepAlive>> keep_alive_for_locks;

    friend class KeepAlive;
    friend class Watcher;
};

}

#endif
