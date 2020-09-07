#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <memory>
#include "etcd/Client.hpp"
#include "etcd/v3/action_constants.hpp"
#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncTxnResponse.hpp"
#include "etcd/v3/AsyncRangeResponse.hpp"
#include "etcd/v3/AsyncWatchResponse.hpp"
#include "etcd/v3/AsyncDeleteRangeResponse.hpp"
#include "etcd/v3/AsyncLockResponse.hpp"
#include "etcd/v3/Transaction.hpp"
#include <iostream>

#include "etcd/v3/AsyncSetAction.hpp"
#include "etcd/v3/AsyncCompareAndSwapAction.hpp"
#include "etcd/v3/AsyncCompareAndDeleteAction.hpp"
#include "etcd/v3/AsyncUpdateAction.hpp"
#include "etcd/v3/AsyncGetAction.hpp"
#include "etcd/v3/AsyncDeleteAction.hpp"
#include "etcd/v3/AsyncWatchAction.hpp"
#include "etcd/v3/AsyncLeaseGrantAction.hpp"
#include "etcd/v3/AsyncLockAction.hpp"
#include "etcd/v3/AsyncTxnAction.hpp"

#include <boost/algorithm/string.hpp>

using grpc::Channel;


static bool dns_resolve(std::string const &target, std::vector<std::string> &endpoints) {
  struct addrinfo hints = {}, *addrs;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  std::vector<std::string> target_parts;
  boost::split(target_parts, target, boost::is_any_of(":"));
  if (target_parts.size() != 2) {
    std::cerr << "warn: invalid URL: " << target << std::endl;
    return false;
  }
  if (getaddrinfo(target_parts[0].c_str(), target_parts[1].c_str(), &hints, &addrs) != 0) {
    std::cerr << "warn: getaddrinfo() failed for endpoint " << target << std::endl;
    return false;
  }

  char host[16] = {'\0'};
  for (struct addrinfo* addr = addrs; addr != nullptr; addr = addr->ai_next) {
    memset(host, '\0', sizeof(host));
    getnameinfo(addr->ai_addr, addr->ai_addrlen, host, sizeof(host), NULL, 0, NI_NUMERICHOST);
    endpoints.emplace_back(std::string(host) + ":" + target_parts[1]);
  }
  freeaddrinfo(addrs);
  return true;
}

etcd::Client::Client(std::string const & address, std::string const & load_balancer)
{
  std::vector<std::string> addresses;
  boost::algorithm::split(addresses, address, boost::algorithm::is_any_of(",;"));
  std::string stripped_address;
  {
    std::vector<std::string> stripped_addresses;
    std::string substr("://");
    for (auto const &addr: addresses) {
      std::string::size_type idx = addr.find(substr);
      std::string target = idx == std::string::npos ? addr : addr.substr(idx + substr.length());
      dns_resolve(target, stripped_addresses);
    }
    stripped_address = boost::algorithm::join(stripped_addresses, ",");
  }
  grpc::ChannelArguments grpc_args;
  grpc_args.SetLoadBalancingPolicyName(load_balancer);
  this->channel = grpc::CreateCustomChannel(
      "ipv4:///" + stripped_address,
      grpc::InsecureChannelCredentials(),
      grpc_args);
  stub_= KV::NewStub(this->channel);
  watchServiceStub= Watch::NewStub(this->channel);
  leaseServiceStub= Lease::NewStub(this->channel);
  lockServiceStub = Lock::NewStub(this->channel);
}


pplx::task<etcd::Response> etcd::Client::get(std::string const & key)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.withPrefix = false;
  params.kv_stub = stub_.get();
  std::shared_ptr<etcdv3::AsyncGetAction> call(new etcdv3::AsyncGetAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::set(std::string const & key, std::string const & value, int ttl)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.kv_stub = stub_.get();

  if(ttl > 0)
  {
    auto res = leasegrant(ttl).get();
    if(!res.is_ok()) 
    {
      return pplx::task<etcd::Response>([res]()
      {
        return etcd::Response(res.error_code(), res.error_message().c_str());    
      });
    }
    else
    {
      params.lease_id = res.value().lease();
    }
  }

  std::shared_ptr<etcdv3::AsyncSetAction> call(new etcdv3::AsyncSetAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::set(std::string const & key, std::string const & value, int64_t leaseid)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.lease_id = leaseid;
  params.kv_stub = stub_.get();
  std::shared_ptr<etcdv3::AsyncSetAction> call(new etcdv3::AsyncSetAction(params));
  return Response::create(call);
}


pplx::task<etcd::Response> etcd::Client::add(std::string const & key, std::string const & value, int ttl)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.kv_stub = stub_.get();

  if(ttl > 0)
  {
    auto res = leasegrant(ttl).get();
    if(!res.is_ok()) 
    {
      return pplx::task<etcd::Response>([res]()
      {
        return etcd::Response(res.error_code(), res.error_message().c_str());    
      });
    }
    else
    {
      params.lease_id = res.value().lease();
    }
  }
  std::shared_ptr<etcdv3::AsyncSetAction> call(new etcdv3::AsyncSetAction(params,true));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::add(std::string const & key, std::string const & value, int64_t leaseid)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.lease_id = leaseid;
  params.kv_stub = stub_.get();
  std::shared_ptr<etcdv3::AsyncSetAction> call(new etcdv3::AsyncSetAction(params,true));
  return Response::create(call);
}


pplx::task<etcd::Response> etcd::Client::modify(std::string const & key, std::string const & value, int ttl)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.kv_stub = stub_.get();

  if(ttl > 0)
  {
    auto res = leasegrant(ttl).get();
    if(!res.is_ok()) 
    {
      return pplx::task<etcd::Response>([res]()
      {
        return etcd::Response(res.error_code(), res.error_message().c_str());    
      });
    }
    else
    {
      params.lease_id = res.value().lease();
    }
  }
  std::shared_ptr<etcdv3::AsyncUpdateAction> call(new etcdv3::AsyncUpdateAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::modify(std::string const & key, std::string const & value, int64_t leaseid)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.lease_id = leaseid;
  params.kv_stub = stub_.get();
  std::shared_ptr<etcdv3::AsyncUpdateAction> call(new etcdv3::AsyncUpdateAction(params));
  return Response::create(call);
}


pplx::task<etcd::Response> etcd::Client::modify_if(std::string const & key, std::string const & value, std::string const & old_value, int ttl)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.old_value.assign(old_value);
  params.kv_stub = stub_.get();

  if(ttl > 0)
  {
    auto res = leasegrant(ttl).get();
    if(!res.is_ok()) 
    {
      return pplx::task<etcd::Response>([res]()
      {
        return etcd::Response(res.error_code(), res.error_message().c_str());    
      });
    }
    else
    {
      params.lease_id = res.value().lease();
    }
  }
  std::shared_ptr<etcdv3::AsyncCompareAndSwapAction> call(new etcdv3::AsyncCompareAndSwapAction(params,etcdv3::Atomicity_Type::PREV_VALUE));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::modify_if(std::string const & key, std::string const & value, std::string const & old_value, int64_t leaseid)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.old_value.assign(old_value);
  params.lease_id = leaseid;
  params.kv_stub = stub_.get();
  std::shared_ptr<etcdv3::AsyncCompareAndSwapAction> call(new etcdv3::AsyncCompareAndSwapAction(params,etcdv3::Atomicity_Type::PREV_VALUE));
  return Response::create(call);
}



pplx::task<etcd::Response> etcd::Client::modify_if(std::string const & key, std::string const & value, int old_index, int ttl)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.old_revision = old_index;
  params.kv_stub = stub_.get();
  if(ttl > 0)
  {
    auto res = leasegrant(ttl).get();
    if(!res.is_ok()) 
    {
      return pplx::task<etcd::Response>([res]()
      {
        return etcd::Response(res.error_code(), res.error_message().c_str());    
      });
    }
    else
    {
      params.lease_id = res.value().lease();
    }
  }
  std::shared_ptr<etcdv3::AsyncCompareAndSwapAction> call(new etcdv3::AsyncCompareAndSwapAction(params,etcdv3::Atomicity_Type::PREV_INDEX));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::modify_if(std::string const & key, std::string const & value, int old_index, int64_t leaseid)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.lease_id = leaseid;
  params.old_revision = old_index;
  params.kv_stub = stub_.get();
  std::shared_ptr<etcdv3::AsyncCompareAndSwapAction> call(new etcdv3::AsyncCompareAndSwapAction(params,etcdv3::Atomicity_Type::PREV_INDEX));
  return Response::create(call);
}


pplx::task<etcd::Response> etcd::Client::rm(std::string const & key)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.withPrefix = false;
  params.kv_stub = stub_.get();
  std::shared_ptr<etcdv3::AsyncDeleteAction> call(new etcdv3::AsyncDeleteAction(params));
  return Response::create(call);
}


pplx::task<etcd::Response> etcd::Client::rm_if(std::string const & key, std::string const & old_value)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.old_value.assign(old_value);
  params.kv_stub = stub_.get();
  std::shared_ptr<etcdv3::AsyncCompareAndDeleteAction> call(new etcdv3::AsyncCompareAndDeleteAction(params,etcdv3::Atomicity_Type::PREV_VALUE));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::rm_if(std::string const & key, int old_index)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.old_revision = old_index;
  params.kv_stub = stub_.get();
  std::shared_ptr<etcdv3::AsyncCompareAndDeleteAction> call(new etcdv3::AsyncCompareAndDeleteAction(params, etcdv3::Atomicity_Type::PREV_INDEX));;
  return Response::create(call);

}

pplx::task<etcd::Response> etcd::Client::rmdir(std::string const & key, bool recursive)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.withPrefix = recursive;
  params.kv_stub = stub_.get();
  std::shared_ptr<etcdv3::AsyncDeleteAction> call(new etcdv3::AsyncDeleteAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::ls(std::string const & key)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.withPrefix = true;
  params.limit = 0;  // default no limit.
  params.kv_stub = stub_.get();
  std::shared_ptr<etcdv3::AsyncGetAction> call(new etcdv3::AsyncGetAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::ls(std::string const & key, size_t const limit)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.withPrefix = true;
  params.limit = limit;
  params.kv_stub = stub_.get();
  std::shared_ptr<etcdv3::AsyncGetAction> call(new etcdv3::AsyncGetAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::watch(std::string const & key, bool recursive)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.withPrefix = recursive;
  params.watch_stub = watchServiceStub.get();
  std::shared_ptr<etcdv3::AsyncWatchAction> call(new etcdv3::AsyncWatchAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::watch(std::string const & key, int fromIndex, bool recursive)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.withPrefix = recursive;
  params.revision = fromIndex;
  params.watch_stub = watchServiceStub.get();
  std::shared_ptr<etcdv3::AsyncWatchAction> call(new etcdv3::AsyncWatchAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::leasegrant(int ttl)
{
  etcdv3::ActionParameters params;
  params.ttl = ttl;
  params.lease_stub = leaseServiceStub.get();
  std::shared_ptr<etcdv3::AsyncLeaseGrantAction> call(new etcdv3::AsyncLeaseGrantAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::lock(std::string const &key) {
  etcdv3::ActionParameters params;
  params.key = key;
  params.lock_stub = lockServiceStub.get();
  std::shared_ptr<etcdv3::AsyncLockAction> call(new etcdv3::AsyncLockAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::unlock(std::string const &key) {
  etcdv3::ActionParameters params;
  params.key = key;
  params.lock_stub = lockServiceStub.get();
  std::shared_ptr<etcdv3::AsyncUnlockAction> call(new etcdv3::AsyncUnlockAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::txn(etcdv3::Transaction const &txn) {
  etcdv3::ActionParameters params;
  params.kv_stub = stub_.get();
  std::shared_ptr<etcdv3::AsyncTxnAction> call(new etcdv3::AsyncTxnAction(params, txn));
  return Response::create(call);
}
