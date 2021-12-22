#include "etcd/SyncClient.hpp"

#define CHECK_EXCEPTIONS(cmd)                   \
  try                                           \
  {                                             \
    return cmd;                                 \
  }                                             \
  catch (std::exception const & ex)             \
  {                                             \
    return etcd::Response(500, ex.what());      \
  }

etcd::SyncClient::SyncClient(std::string const & address, std::string const & load_balancer)
  : client(address, load_balancer)
{
}

etcd::SyncClient::SyncClient(std::string const & address,
                             std::string const & username,
                             std::string const & password,
                             const int auth_token_ttl,
                             std::string const & load_balancer)
  : client(address, username, password, auth_token_ttl, load_balancer)
{
}

etcd::Response etcd::SyncClient::head()
{
  CHECK_EXCEPTIONS(client.head().get());
}

etcd::Response etcd::SyncClient::get(std::string const & key)
{
  CHECK_EXCEPTIONS(client.get(key).get());
}

etcd::Response etcd::SyncClient::set(std::string const & key, std::string const & value, int ttl)
{
  CHECK_EXCEPTIONS(client.set(key, value, ttl).get());
}

etcd::Response etcd::SyncClient::set(std::string const & key, std::string const & value, int64_t leaseId)
{
  CHECK_EXCEPTIONS(client.set(key, value, leaseId).get());
}

etcd::Response etcd::SyncClient::add(std::string const & key, std::string const & value, int ttl)
{
  CHECK_EXCEPTIONS(client.add(key, value, ttl).get());
}

etcd::Response etcd::SyncClient::add(std::string const & key, std::string const & value, int64_t leaseId)
{
  CHECK_EXCEPTIONS(client.add(key, value, leaseId).get());
}

etcd::Response etcd::SyncClient::put(std::string const & key, std::string const & value)
{
  CHECK_EXCEPTIONS(client.put(key, value).get());
}

etcd::Response etcd::SyncClient::modify(std::string const & key, std::string const & value, int ttl)
{
  CHECK_EXCEPTIONS(client.modify(key, value, ttl).get());
}

etcd::Response etcd::SyncClient::modify(std::string const & key, std::string const & value, int64_t leaseId)
{
  CHECK_EXCEPTIONS(client.modify(key, value, leaseId).get());
}

etcd::Response etcd::SyncClient::modify_if(std::string const & key, std::string const & value, std::string const & old_value, int ttl)
{
  CHECK_EXCEPTIONS(client.modify_if(key, value, old_value, ttl).get());
}

etcd::Response etcd::SyncClient::modify_if(std::string const & key, std::string const & value, std::string const & old_value, int64_t leaseId)
{
  CHECK_EXCEPTIONS(client.modify_if(key, value, old_value, leaseId).get());
}

etcd::Response etcd::SyncClient::modify_if(std::string const & key, std::string const & value, int64_t old_index, int ttl)
{
  CHECK_EXCEPTIONS(client.modify_if(key, value, old_index, ttl).get());
}

etcd::Response etcd::SyncClient::modify_if(std::string const & key, std::string const & value, int64_t old_index, int64_t leaseId)
{
  CHECK_EXCEPTIONS(client.modify_if(key, value, old_index, leaseId).get());
}

etcd::Response etcd::SyncClient::rm(std::string const & key)
{
  CHECK_EXCEPTIONS(client.rm(key).get());
}

etcd::Response etcd::SyncClient::rm_if(std::string const & key, std::string const & old_value)
{
  CHECK_EXCEPTIONS(client.rm_if(key, old_value).get());
}

etcd::Response etcd::SyncClient::rm_if(std::string const & key, int64_t old_index)
{
  CHECK_EXCEPTIONS(client.rm_if(key, old_index).get());
}


etcd::Response etcd::SyncClient::rmdir(std::string const & key, bool recursive)
{
  CHECK_EXCEPTIONS(client.rmdir(key, recursive).get());
}

etcd::Response etcd::SyncClient::rmdir(std::string const & key, const char *range_end)
{
  CHECK_EXCEPTIONS(client.rmdir(key, range_end).get());
}

etcd::Response etcd::SyncClient::rmdir(std::string const & key, std::string const &range_end)
{
  CHECK_EXCEPTIONS(client.rmdir(key, range_end).get());
}

etcd::Response etcd::SyncClient::ls(std::string const & key)
{
  CHECK_EXCEPTIONS(client.ls(key).get());
}

etcd::Response etcd::SyncClient::ls(std::string const & key, size_t const limit)
{
  CHECK_EXCEPTIONS(client.ls(key, limit).get());
}

etcd::Response etcd::SyncClient::ls(std::string const & key, std::string const &range_end)
{
  CHECK_EXCEPTIONS(client.ls(key, range_end).get());
}

etcd::Response etcd::SyncClient::ls(std::string const & key, std::string const &range_end, size_t limit)
{
  CHECK_EXCEPTIONS(client.ls(key, range_end, limit).get());
}

etcd::Response etcd::SyncClient::leasegrant(int ttl)
{
  CHECK_EXCEPTIONS(client.leasegrant(ttl).get());
}

etcd::Response etcd::SyncClient::leaserevoke(int64_t lease_id)
{
  CHECK_EXCEPTIONS(client.leaserevoke(lease_id).get());
}

etcd::Response etcd::SyncClient::leasetimetolive(int64_t lease_id)
{
  CHECK_EXCEPTIONS(client.leasetimetolive(lease_id).get());
}

etcd::Response etcd::SyncClient::campaign(std::string const &name, int64_t lease_id,
                                          std::string const &value)
{
  CHECK_EXCEPTIONS(client.campaign(name, lease_id, value).get());
}

etcd::Response etcd::SyncClient::proclaim(std::string const &name, int64_t lease_id,
                                          std::string const &key, int64_t revision,
                                          std::string const &value)
{
  CHECK_EXCEPTIONS(client.proclaim(name, lease_id, key, revision, value).get());
}

etcd::Response etcd::SyncClient::leader(std::string const &name)
{
  CHECK_EXCEPTIONS(client.leader(name).get());
}

std::unique_ptr<etcd::Client::Observer> etcd::SyncClient::observe(
    std::string const &name, const bool once)
{
  return client.observe(name, once);
}

std::unique_ptr<etcd::Client::Observer> etcd::SyncClient::observe(
    std::string const &name,
    std::function<void(etcd::Response)> callback,
    const bool once)
{
  return client.observe(name, callback, once);
}

etcd::Response etcd::SyncClient::resign(std::string const &name, int64_t lease_id,
                                        std::string const &key, int64_t revision)
{
  CHECK_EXCEPTIONS(client.resign(name, lease_id, key, revision).get());
}

etcd::Response etcd::SyncClient::watch(std::string const & key, bool recursive)
{
  CHECK_EXCEPTIONS(client.watch(key, recursive).get());
}

etcd::Response etcd::SyncClient::watch(std::string const & key, int64_t fromIndex, bool recursive)
{
  CHECK_EXCEPTIONS(client.watch(key, fromIndex, recursive).get());
}

etcd::Response etcd::SyncClient::watch(std::string const & key, const char *range_end)
{
  CHECK_EXCEPTIONS(client.watch(key, range_end).get());
}

etcd::Response etcd::SyncClient::watch(std::string const & key, std::string const &range_end)
{
  CHECK_EXCEPTIONS(client.watch(key, range_end).get());
}

etcd::Response etcd::SyncClient::watch(std::string const & key, std::string const &range_end, int64_t fromIndex)
{
  CHECK_EXCEPTIONS(client.watch(key, range_end, fromIndex).get());
}
