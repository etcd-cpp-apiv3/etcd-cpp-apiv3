#ifndef __ETCD_SYNC_CLIENT_HPP__
#define __ETCD_SYNC_CLIENT_HPP__

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <ratio>
#include <string>

#include "etcd/Response.hpp"
#include "etcd/v3/action_constants.hpp"

namespace etcdv3 {
struct ActionParameters;

class AsyncCompareAndDeleteAction;
class AsyncCompareAndSwapAction;
class AsyncDeleteAction;
class AsyncCampaignAction;
class AsyncProclaimAction;
class AsyncLeaderAction;
class AsyncObserveAction;
class AsyncResignAction;
class AsyncHeadAction;
class AsyncLeaseGrantAction;
class AsyncLeaseRevokeAction;
class AsyncLeaseKeepAliveAction;
class AsyncLeaseTimeToLiveAction;
class AsyncLeaseLeasesAction;
class AsyncLockAction;
class AsyncUnlockAction;
class AsyncPutAction;
class AsyncRangeAction;
class AsyncSetAction;
class AsyncTxnAction;
class AsyncUpdateAction;
class AsyncWatchAction;

enum class AtomicityType;
class Transaction;

namespace detail {
std::string string_plus_one(std::string const& value);
std::string resolve_etcd_endpoints(std::string const& default_endpoints);
}  // namespace detail
}  // namespace etcdv3

#if defined(WITH_GRPC_CHANNEL_CLASS)
namespace grpc {
class Channel;
class ChannelArguments;
}  // namespace grpc
#else
namespace grpc_impl {
class Channel;
class ChannelArguments;
}  // namespace grpc_impl
#endif

namespace etcd {
using etcdv3::ERROR_ACTION_CANCELLED;
using etcdv3::ERROR_COMPARE_FAILED;
using etcdv3::ERROR_KEY_ALREADY_EXISTS;
using etcdv3::ERROR_KEY_NOT_FOUND;

class KeepAlive;
class Watcher;
class Client;

/**
 * Client is responsible for maintaining a connection towards an etcd server.
 * Etcd operations can be reached via the methods of the client.
 */
/**
 * SyncClient is a wrapper around etcd::Client and provides a simplified sync
 * interface with blocking operations.
 *
 * In case of any exceptions occur in the backend the Response value's
 * error_message will contain the text of the exception and the error code will
 * be 600
 *
 * Use this class only if performance does not matter.
 */
class SyncClient {
 private:
  class TokenAuthenticator;
  class TokenAuthenticatorDeleter {
   public:
    void operator()(TokenAuthenticator* authenticator);
  };

 public:
  /**
   * Constructs an etcd client object.
   *
   * @param etcd_url is the url of the etcd server to connect to, like
   * "http://127.0.0.1:2379", or multiple url, separated by ',' or ';'.
   * @param load_balancer is the load balance strategy, can be one of
   * round_robin/pick_first/grpclb/xds.
   */
  SyncClient(std::string const& etcd_url,
             std::string const& load_balancer = "round_robin");

  /**
   * Constructs an etcd client object.
   *
   * @param etcd_url is the url of the etcd server to connect to, like
   * "http://127.0.0.1:2379", or multiple url, separated by ',' or ';'.
   * @param arguments user provided grpc channel arguments.
   */
  SyncClient(std::string const& etcd_url,
#if defined(WITH_GRPC_CHANNEL_CLASS)
             grpc::ChannelArguments const& arguments
#else
             grpc_impl::ChannelArguments const& arguments
#endif
  );

  /**
   * Constructs an etcd client object.
   *
   * @param etcd_url is the url of the etcd server to connect to, like
   * "http://127.0.0.1:2379", or multiple url, separated by ',' or ';'.
   * @param load_balancer is the load balance strategy, can be one of
   * round_robin/pick_first/grpclb/xds.
   */
  static SyncClient* WithUrl(std::string const& etcd_url,
                             std::string const& load_balancer = "round_robin");

  /**
   * Constructs an etcd client object.
   *
   * @param etcd_url is the url of the etcd server to connect to, like
   * "http://127.0.0.1:2379", or multiple url, separated by ',' or ';'.
   * @param arguments user provided grpc channel arguments.
   */
  static SyncClient* WithUrl(std::string const& etcd_url,
#if defined(WITH_GRPC_CHANNEL_CLASS)
                             grpc::ChannelArguments const& arguments
#else
                             grpc_impl::ChannelArguments const& arguments
#endif
  );

  /**
   * Constructs an etcd client object.
   *
   * @param etcd_url is the url of the etcd server to connect to, like
   * "http://127.0.0.1:2379", or multiple url, separated by ',' or ';'.
   * @param username username of etcd auth
   * @param password password of etcd auth
   * @param load_balancer is the load balance strategy, can be one of
   * round_robin/pick_first/grpclb/xds.
   * @param auth_token_ttl TTL seconds for auth token, see also
   * `--auth-token-ttl` flags of etcd.
   */
  SyncClient(std::string const& etcd_url, std::string const& username,
             std::string const& password, int const auth_token_ttl = 300,
             std::string const& load_balancer = "round_robin");

  /**
   * Constructs an etcd client object.
   *
   * @param etcd_url is the url of the etcd server to connect to, like
   * "http://127.0.0.1:2379", or multiple url, separated by ',' or ';'.
   * @param username username of etcd auth
   * @param password password of etcd auth
   * @param arguments user provided grpc channel arguments.
   * @param auth_token_ttl TTL seconds for auth token, see also
   * `--auth-token-ttl` flags of etcd. Default value should be 300.
   */
  SyncClient(std::string const& etcd_url, std::string const& username,
             std::string const& password, int const auth_token_ttl,
#if defined(WITH_GRPC_CHANNEL_CLASS)
             grpc::ChannelArguments const& arguments
#else
             grpc_impl::ChannelArguments const& arguments
#endif
  );

  /**
   * Constructs an etcd client object.
   *
   * @param etcd_url is the url of the etcd server to connect to, like
   * "http://127.0.0.1:2379", or multiple url, separated by ',' or ';'.
   * @param username username of etcd auth
   * @param password password of etcd auth
   * @param load_balancer is the load balance strategy, can be one of
   * round_robin/pick_first/grpclb/xds.
   * @param auth_token_ttl TTL seconds for auth token, see also
   * `--auth-token-ttl` flags of etcd.
   */
  static SyncClient* WithUser(std::string const& etcd_url,
                              std::string const& username,
                              std::string const& password,
                              int const auth_token_ttl = 300,
                              std::string const& load_balancer = "round_robin");

  /**
   * Constructs an etcd client object.
   *
   * @param etcd_url is the url of the etcd server to connect to, like
   * "http://127.0.0.1:2379", or multiple url, separated by ',' or ';'.
   * @param username username of etcd auth
   * @param password password of etcd auth
   * @param arguments user provided grpc channel arguments.
   * @param auth_token_ttl TTL seconds for auth token, see also
   * `--auth-token-ttl` flags of etcd. Default value should be 300.
   */
  static SyncClient* WithUser(std::string const& etcd_url,
                              std::string const& username,
                              std::string const& password,
                              int const auth_token_ttl,
#if defined(WITH_GRPC_CHANNEL_CLASS)
                              grpc::ChannelArguments const& arguments
#else
                              grpc_impl::ChannelArguments const& arguments
#endif
  );

  /**
   * Constructs an etcd client object.
   *
   * @param etcd_url is the url of the etcd server to connect to, like
   * "http://127.0.0.1:2379", or multiple url, separated by ',' or ';'.
   * @param ca      root CA file for SSL/TLS connection.
   * @param cert    cert chain file for SSL/TLS authentication, could be empty
   * string.
   * @param privkey private key file for SSL/TLS authentication, could be empty
   * string.
   * @param load_balancer is the load balance strategy, can be one of
   * round_robin/pick_first/grpclb/xds.
   */
  SyncClient(std::string const& etcd_url, std::string const& ca,
             std::string const& cert, std::string const& privkey,
             std::string const& target_name_override,
             std::string const& load_balancer = "round_robin");

  /**
   * Constructs an etcd client object.
   *
   * @param etcd_url is the url of the etcd server to connect to, like
   * "http://127.0.0.1:2379", or multiple url, separated by ',' or ';'.
   * @param ca      root CA file for SSL/TLS connection.
   * @param cert    cert chain file for SSL/TLS authentication, could be empty
   * string.
   * @param privkey private key file for SSL/TLS authentication, could be empty
   * string.
   * @param arguments user provided grpc channel arguments.
   */
  SyncClient(std::string const& etcd_url, std::string const& ca,
             std::string const& cert, std::string const& privkey,
             std::string const& target_name_override,
#if defined(WITH_GRPC_CHANNEL_CLASS)
             grpc::ChannelArguments const& arguments
#else
             grpc_impl::ChannelArguments const& arguments
#endif
  );

  /**
   * Constructs an etcd client object.
   *
   * @param etcd_url is the url of the etcd server to connect to, like
   * "http://127.0.0.1:2379", or multiple url, separated by ',' or ';'.
   * @param ca      root CA file for SSL/TLS connection.
   * @param cert    cert chain file for SSL/TLS authentication, could be empty
   * string.
   * @param privkey private key file for SSL/TLS authentication, could be empty
   * string.
   * @param target_name_override Override the target host name if you want to
   * pass multiple address for load balancing with SSL, and there's no DNS. The
   * @target_name_override@ must exist in the SANS of your SSL certificate.
   * @param load_balancer is the load balance strategy, can be one of
   * round_robin/pick_first/grpclb/xds.
   */
  static SyncClient* WithSSL(std::string const& etcd_url, std::string const& ca,
                             std::string const& cert = "",
                             std::string const& privkey = "",
                             std::string const& target_name_override = "",
                             std::string const& load_balancer = "round_robin");

  /**
   * Constructs an etcd client object.
   *
   * @param etcd_url is the url of the etcd server to connect to, like
   * "http://127.0.0.1:2379", or multiple url, separated by ',' or ';'.
   * @param ca      root CA file for SSL/TLS connection.
   * @param cert    cert chain file for SSL/TLS authentication, could be empty
   * string.
   * @param privkey private key file for SSL/TLS authentication, could be empty
   * string.
   * @param target_name_override Override the target host name if you want to
   * pass multiple address for load balancing with SSL, and there's no DNS. The
   * @target_name_override@ must exist in the SANS of your SSL certificate.
   * @param arguments user provided grpc channel arguments.
   */
  static SyncClient* WithSSL(std::string const& etcd_url,
#if defined(WITH_GRPC_CHANNEL_CLASS)
                             grpc::ChannelArguments const& arguments,
#else
                             grpc_impl::ChannelArguments const& arguments,
#endif
                             std::string const& ca,
                             std::string const& cert = "",
                             std::string const& privkey = "",
                             std::string const& target_name_override = "");

  ~SyncClient();

  /**
   * Get the HEAD revision of the connected etcd server.
   */
  Response head();

  /**
   * Get the value of specified key from the etcd server
   * @param key is the key to be read
   */
  Response get(std::string const& key);

  /**
   * Get the value of specified key of specified revision from the etcd server
   * @param key is the key to be read
   * @param revision is the revision of the key to be read
   */
  Response get(std::string const& key, int64_t revision);

  /**
   * Sets the value of a key. The key will be modified if already exists or
   * created if it does not exists.
   * @param key is the key to be created or modified
   * @param value is the new value to be set
   * @param leaseId is the lease attached to the key
   */
  Response set(std::string const& key, std::string const& value,
               const int64_t leaseId = 0);

  /**
   * Creates a new key and sets it's value. Fails if the key already exists.
   * @param key is the key to be created
   * @param value is the value to be set
   * @param leaseId is the lease attached to the key
   */
  Response add(std::string const& key, std::string const& value,
               const int64_t leaseId = 0);

  /**
   * Put a new key-value pair.
   * @param key is the key to be put
   * @param value is the value to be put
   */
  Response put(std::string const& key, std::string const& value);

  /**
   * Put a new key-value pair.
   * @param key is the key to be put
   * @param value is the value to be put
   * @param leaseId is the lease id to be associated with the key
   */
  Response put(std::string const& key, std::string const& value,
               const int64_t leaseId);

  /**
   * Modifies an existing key. Fails if the key does not exists.
   * @param key is the key to be modified
   * @param value is the new value to be set
   * @param leaseId is the lease attached to the key
   */
  Response modify(std::string const& key, std::string const& value,
                  const int64_t leaseId = 0);

  /**
   * Modifies an existing key only if it has a specific value. Fails if the key
   * does not exists or the original value differs from the expected one.
   * @param key is the key to be modified
   * @param value is the new value to be set
   * @param old_value is the value to be replaced
   * @param leaseId is the lease attached to the key
   */
  Response modify_if(std::string const& key, std::string const& value,
                     std::string const& old_value, const int64_t leaseId = 0);

  /**
   * Modifies an existing key only if it has a specific modification index
   * value. Fails if the key does not exists or the modification index of the
   * previous value differs from the expected one.
   * @param key is the key to be modified
   * @param value is the new value to be set
   * @param old_index is the expected index of the original value
   * @param leaseId is the lease attached to the key
   */
  Response modify_if(std::string const& key, std::string const& value,
                     int64_t old_index, const int64_t leaseId = 0);

  /**
   * Removes a single key. The key has to point to a plain, non directory entry.
   * @param key is the key to be deleted
   *
   * @return Returns etcdv3::ERROR_KEY_NOT_FOUND if the key does not exist.
   */
  Response rm(std::string const& key);

  /**
   * Removes a single key but only if it has a specific value. Fails if the key
   * does not exists or the its value differs from the expected one.
   * @param key is the key to be deleted
   *
   * @return Returns etcdv3::ERROR_KEY_NOT_FOUND if the key does not exist.
   */
  Response rm_if(std::string const& key, std::string const& old_value);

  /**
   * Removes an existing key only if it has a specific modification index value.
   * Fails if the key does not exists or the modification index of it differs
   * from the expected one.
   * @param key is the key to be deleted
   * @param old_index is the expected index of the existing value
   *
   * @return Returns etcdv3::ERROR_KEY_NOT_FOUND if the key does not exist.
   */
  Response rm_if(std::string const& key, int64_t old_index);

  /**
   * Removes a directory node. Fails if the parent directory dos not exists or
   * not a directory.
   * @param key is the directory to be created to be listed
   * @param recursive if true then delete a whole subtree, otherwise deletes
   * only an empty directory.
   *
   * @return Returns etcdv3::ERROR_KEY_NOT_FOUND if the no key been deleted.
   */
  Response rmdir(std::string const& key, bool recursive = false);

  /**
   * Removes multiple keys between [key, range_end).
   *
   * This overload for `const char *` is to avoid const char * to bool implicit
   * casting.
   *
   * @param key is the directory to be created to be listed
   * @param range_end is the end of key range to be removed.
   *
   * @return Returns etcdv3::ERROR_KEY_NOT_FOUND if the no key been deleted.
   */
  Response rmdir(std::string const& key, const char* range_end);

  /**
   * Removes multiple keys between [key, range_end).
   *
   * @param key is the directory to be created to be listed
   * @param range_end is the end of key range to be removed.
   *
   * @return Returns etcdv3::ERROR_KEY_NOT_FOUND if the no key been deleted.
   */
  Response rmdir(std::string const& key, std::string const& range_end);

  /**
   * Gets a directory listing of the directory prefixed by the key.
   *
   * @param key is the key to be listed
   */
  Response ls(std::string const& key);

  /**
   * Gets a directory listing of the directory prefixed by the key.
   *
   * @param key is the key to be listed
   * @param limit is the size limit of results to be listed, we don't use
   * default parameters to ensure backwards binary compatibility. 0 means no
   * limit.
   */
  Response ls(std::string const& key, size_t const limit);

  /**
   * Gets a directory listing of the directory prefixed by the key with given
   * revision.
   *
   * @param key is the key to be listed
   * @param limit is the size limit of results to be listed, we don't use
   * default parameters to ensure backwards binary compatibility. 0 means no
   * limit.
   * @param revision is the revision to be listed
   */
  Response ls(std::string const& key, size_t const limit, int64_t revision);

  /**
   * Gets a directory listing of the directory identified by the key and
   * range_end, i.e., get all keys in the range [key, range_end).
   *
   * @param key is the key to be listed
   * @param range_end is the end of key range to be listed
   */
  Response ls(std::string const& key, std::string const& range_end);

  /**
   * Gets a directory listing of the directory identified by the key and
   * range_end, i.e., get all keys in the range [key, range_end), and respects
   * the given limit.
   *
   * @param key is the key to be listed
   * @param range_end is the end of key range to be listed
   * @param limit is the size limit of results to be listed, we don't use
   * default parameters to ensure backwards binary compatibility. 0 means no
   * limit.
   */
  Response ls(std::string const& key, std::string const& range_end,
              size_t const limit);

  /**
   * Gets a directory listing of the directory identified by the key and
   * range_end, i.e., get all keys in the range [key, range_end), and respects
   * the given limit and revision.
   *
   * @param key is the key to be listed
   * @param range_end is the end of key range to be listed
   * @param limit is the size limit of results to be listed, we don't use
   * default parameters to ensure backwards binary compatibility. 0 means no
   * limit.
   * @param revision is the revision to be listed
   */
  Response ls(std::string const& key, std::string const& range_end,
              size_t const limit, int64_t revision);

  /**
   * Gets a directory listing of the directory prefixed by the key.
   *
   * Note that only keys are included in the response.
   *
   * @param key is the key to be listed
   */
  Response keys(std::string const& key);

  /**
   * Gets a directory listing of the directory prefixed by the key.
   *
   * Note that only keys are included in the response.
   *
   * @param key is the key to be listed
   * @param limit is the size limit of results to be listed, we don't use
   * default parameters to ensure backwards binary compatibility.
   */
  Response keys(std::string const& key, size_t const limit);

  /**
   * Gets a directory listing of the directory prefixed by the key with
   * specified revision.
   *
   * Note that only keys are included in the response.
   *
   * @param key is the key to be listed
   * @param limit is the size limit of results to be listed, we don't use
   * default parameters to ensure backwards binary compatibility.
   * @param revision is the revision to be listed
   */
  Response keys(std::string const& key, size_t const limit, int64_t revision);

  /**
   * List keys identified by the key and range_end, i.e., get all keys in the
   * range [key, range_end).
   *
   * Note that only keys are included in the response.
   *
   * @param key is the key to be listed
   * @param range_end is the end of key range to be listed
   */
  Response keys(std::string const& key, std::string const& range_end);

  /**
   * List keys identified by the key and range_end, i.e., get all keys in the
   * range [key, range_end), and respects the given limit.
   *
   * Note that only keys are included in the response.
   *
   * @param key is the key to be listed
   * @param range_end is the end of key range to be listed
   * @param limit is the size limit of results to be listed, we don't use
   * default parameters to ensure backwards binary compatibility.
   */
  Response keys(std::string const& key, std::string const& range_end,
                size_t const limit);

  /**
   * List keys identified by the key and range_end, i.e., get all keys in the
   * range [key, range_end), and respects the given limit and revision.
   *
   * Note that only keys are included in the response.
   *
   * @param key is the key to be listed
   * @param range_end is the end of key range to be listed
   * @param limit is the size limit of results to be listed, we don't use
   * default parameters to ensure backwards binary compatibility.
   * @param revision is the revision to be listed
   */
  Response keys(std::string const& key, std::string const& range_end,
                size_t const limit, int64_t revision);

  /**
   * Watches for changes of a key or a subtree. Please note that if you watch
   * e.g. "/testdir" and a new key is created, like "/testdir/newkey" then no
   * change happened in the value of
   * "/testdir" so your watch will not detect this. If you want to detect
   * addition and deletion of directory entries then you have to do a recursive
   * watch.
   * @param key is the value or directory to be watched
   * @param recursive if true watch a whole subtree
   */
  Response watch(std::string const& key, bool recursive = false);

  /**
   * Watches for changes of a key or a subtree from a specific index. The index
   * value can be in the "past".
   * @param key is the value or directory to be watched
   * @param fromIndex the first index we are interested in
   * @param recursive if true watch a whole subtree
   */
  Response watch(std::string const& key, int64_t fromIndex,
                 bool recursive = false);

  /**
   * Watches for changes of a range of keys inside [key, range_end).
   *
   * This overload for `const char *` is to avoid const char * to bool implicit
   * casting.
   *
   * @param key is the value or directory to be watched
   * @param range_end is the end of key range to be removed.
   */
  Response watch(std::string const& key, const char* range_end);

  /**
   * Watches for changes of a range of keys inside [key, range_end).
   *
   * @param key is the value or directory to be watched
   * @param range_end is the end of key range to be removed.
   */
  Response watch(std::string const& key, std::string const& range_end);

  /**
   * Watches for changes of a range of keys inside [key, range_end) from a
   * specific index. The index value can be in the "past".
   *
   * Watches for changes of a key or a subtree from a specific index. The index
   * value can be in the "past".
   * @param key is the value or directory to be watched
   * @param range_end is the end of key range to be removed.
   * @param fromIndex the first index we are interested in
   */
  Response watch(std::string const& key, std::string const& range_end,
                 int64_t fromIndex);

  /**
   * Grants a lease.
   * @param ttl is the time to live of the lease
   */
  Response leasegrant(int ttl);

  /**
   * Grants a lease.
   * @param ttl is the time to live of the lease
   */
  std::shared_ptr<KeepAlive> leasekeepalive(int ttl);

  /**
   * Revoke a lease.
   * @param lease_id is the id the lease
   */
  Response leaserevoke(int64_t lease_id);

  /**
   * Get time-to-live of a lease.
   * @param lease_id is the id the lease
   */
  Response leasetimetolive(int64_t lease_id);

  /**
   * List all alive leases, equivalent to `etcdctl lease list`.
   */
  Response leases();

  /**
   * Gains a lock at a key, using a default created lease, using the default
   * lease (10 seconds), with keeping alive has already been taken care of by
   * the library.
   * @param key is the key to be used to request the lock.
   */
  Response lock(std::string const& key);

  /**
   * Gains a lock at a key, using a default created lease, using the specified
   * lease TTL (in seconds), with keeping alive has already been taken care of
   * by the library.
   * @param key is the key to be used to request the lock.
   * @param lease_ttl is the TTL used to create a lease for the key.
   */
  Response lock(std::string const& key, int lease_ttl);

  /**
   * Gains a lock at a key, using a user-provided lease, the lifetime of the
   * lease won't be taken care of by the library.
   * @param key is the key to be used to request the lock.
   */
  Response lock_with_lease(std::string const& key, int64_t lease_id);

  /**
   * Releases a lock at a key.
   * @param key is the lock key to release.
   */
  Response unlock(std::string const& lock_key);

  /**
   * Execute a etcd transaction.
   * @param txn is the transaction object to be executed.
   */
  Response txn(etcdv3::Transaction const& txn);

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
  Response campaign(std::string const& name, int64_t lease_id,
                    std::string const& value);

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
  Response proclaim(std::string const& name, int64_t lease_id,
                    std::string const& key, int64_t revision,
                    std::string const& value);

  /**
   * Get the current leader proclamation.
   *
   * @param name is the names of election.
   *
   * @returns current election key and value.
   */
  Response leader(std::string const& name);

  /**
   * An observer that will cancel the associated election::observe request
   * when being destruct.
   */
  class Observer {
   public:
    ~Observer();
    // wait at least *one* response from the observer.
    Response WaitOnce();

   private:
    std::shared_ptr<etcdv3::AsyncObserveAction> action = nullptr;

    friend class SyncClient;
  };

  /**
   * Observe the leader change.
   *
   * @param name is the names of election to watch.
   *
   * @returns an observer that holds that action and will cancel the request
   * when being destructed.
   */
  std::unique_ptr<Observer> observe(std::string const& name);

  /**
   * Updates the value of election with a new value, with leader key returns by
   * @campaign@.
   *
   * @param name is the name of election
   * @param lease_id is the user-provided lease id for the proclamation
   * @param key is the generated associated key returned by @campaign@
   * @param revision is the created revision of key-value returned by @campaign@
   */
  Response resign(std::string const& name, int64_t lease_id,
                  std::string const& key, int64_t revision);

 private:
  // TODO: use std::unique_ptr<>
  std::shared_ptr<etcdv3::AsyncHeadAction> head_internal();
  std::shared_ptr<etcdv3::AsyncRangeAction> get_internal(
      std::string const& key, const int64_t revision = 0);
  std::shared_ptr<etcdv3::AsyncSetAction> add_internal(
      std::string const& key, std::string const& value,
      const int64_t leaseId = 0);
  std::shared_ptr<etcdv3::AsyncPutAction> put_internal(
      std::string const& key, std::string const& value,
      const int64_t leaseId = 0);
  std::shared_ptr<etcdv3::AsyncUpdateAction> modify_internal(
      std::string const& key, std::string const& value,
      const int64_t leaseId = 0);
  std::shared_ptr<etcdv3::AsyncCompareAndSwapAction> modify_if_internal(
      std::string const& key, std::string const& value, int64_t old_index,
      std::string const& old_value, etcdv3::AtomicityType const& atomicity_type,
      const int64_t leaseId = 0);
  std::shared_ptr<etcdv3::AsyncDeleteAction> rm_internal(
      std::string const& key);
  std::shared_ptr<etcdv3::AsyncCompareAndDeleteAction> rm_if_internal(
      std::string const& key, int64_t old_index, const std::string& old_value,
      etcdv3::AtomicityType const& atomicity_type);
  std::shared_ptr<etcdv3::AsyncDeleteAction> rmdir_internal(
      std::string const& key, bool recursive = false);
  std::shared_ptr<etcdv3::AsyncDeleteAction> rmdir_internal(
      std::string const& key, std::string const& range_end);
  std::shared_ptr<etcdv3::AsyncRangeAction> ls_internal(
      std::string const& key, size_t const limit, bool const keys_only = false,
      int64_t revision = 0);
  std::shared_ptr<etcdv3::AsyncRangeAction> ls_internal(
      std::string const& key, std::string const& range_end, size_t const limit,
      bool const keys_only = false, int64_t revision = 0);
  std::shared_ptr<etcdv3::AsyncWatchAction> watch_internal(
      std::string const& key, int64_t fromIndex, bool recursive = false);
  std::shared_ptr<etcdv3::AsyncWatchAction> watch_internal(
      std::string const& key, std::string const& range_end, int64_t fromIndex);
  std::shared_ptr<etcdv3::AsyncLeaseRevokeAction> leaserevoke_internal(
      int64_t lease_id);
  std::shared_ptr<etcdv3::AsyncLeaseTimeToLiveAction> leasetimetolive_internal(
      int64_t lease_id);
  std::shared_ptr<etcdv3::AsyncLeaseLeasesAction> leases_internal();
  Response lock_internal(std::string const& key,
                         std::shared_ptr<etcd::KeepAlive> const& keepalive);
  std::shared_ptr<etcdv3::AsyncLockAction> lock_with_lease_internal(
      std::string const& key, int64_t lease_id);
  std::shared_ptr<etcdv3::AsyncUnlockAction> unlock_internal(
      std::string const& lock_key);
  std::shared_ptr<etcdv3::AsyncTxnAction> txn_internal(
      etcdv3::Transaction const& txn);
  std::shared_ptr<etcdv3::AsyncCampaignAction> campaign_internal(
      std::string const& name, int64_t lease_id, std::string const& value);
  std::shared_ptr<etcdv3::AsyncProclaimAction> proclaim_internal(
      std::string const& name, int64_t lease_id, std::string const& key,
      int64_t revision, std::string const& value);
  std::shared_ptr<etcdv3::AsyncLeaderAction> leader_internal(
      std::string const& name);
  std::shared_ptr<etcdv3::AsyncResignAction> resign_internal(
      std::string const& name, int64_t lease_id, std::string const& key,
      int64_t revision);

 public:
  /**
   * Return current auth token.
   */
  const std::string& current_auth_token() const;

  /**
   * Obtain the underlying gRPC channel.
   */
#if defined(WITH_GRPC_CHANNEL_CLASS)
  std::shared_ptr<grpc::Channel> grpc_channel() const;
#else
  std::shared_ptr<grpc_impl::Channel> grpc_channel() const;
#endif

  /**
   * Set a timeout value for grpc operations.
   */
  template <typename Period = std::micro>
  void set_grpc_timeout(std::chrono::duration<std::chrono::microseconds::rep,
                                              Period> const& timeout) {
    grpc_timeout =
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout);
  }

  /**
   * Get the current timeout value for grpc operations.
   */
  std::chrono::microseconds get_grpc_timeout() const {
    return this->grpc_timeout;
  }

 private:
#if defined(WITH_GRPC_CHANNEL_CLASS)
  std::shared_ptr<grpc::Channel> channel;
#else
  std::shared_ptr<grpc_impl::Channel> channel;
#endif

  mutable std::unique_ptr<TokenAuthenticator, TokenAuthenticatorDeleter>
      token_authenticator;
  mutable std::chrono::microseconds grpc_timeout =
      std::chrono::microseconds::zero();

  struct EtcdServerStubs;
  struct EtcdServerStubsDeleter {
    void operator()(EtcdServerStubs* stubs);
  };
  std::unique_ptr<EtcdServerStubs, EtcdServerStubsDeleter> stubs;

  std::mutex mutex_for_keepalives;
  std::map<std::string, int64_t> leases_for_locks;
  std::map<int64_t, std::shared_ptr<KeepAlive>> keep_alive_for_locks;

  friend class KeepAlive;
  friend class Watcher;
  friend class Client;
};

}  // namespace etcd

#endif
