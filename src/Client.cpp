#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#endif

#include <chrono>
#include <iostream>
#include <fstream>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>

#include <boost/algorithm/string.hpp>

#include <grpc++/grpc++.h>
#include <grpc++/security/credentials.h>
#include "proto/rpc.grpc.pb.h"
#include "proto/v3lock.grpc.pb.h"
#include "proto/v3election.grpc.pb.h"

#include "etcd/Client.hpp"
#include "etcd/KeepAlive.hpp"
#include "etcd/v3/action_constants.hpp"
#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncRangeResponse.hpp"
#include "etcd/v3/AsyncWatchResponse.hpp"
#include "etcd/v3/AsyncDeleteResponse.hpp"
#include "etcd/v3/AsyncPutResponse.hpp"
#include "etcd/v3/AsyncLockResponse.hpp"
#include "etcd/v3/AsyncElectionResponse.hpp"
#include "etcd/v3/AsyncTxnResponse.hpp"
#include "etcd/v3/Transaction.hpp"

#include "etcd/v3/AsyncSetAction.hpp"
#include "etcd/v3/AsyncCompareAndSwapAction.hpp"
#include "etcd/v3/AsyncCompareAndDeleteAction.hpp"
#include "etcd/v3/AsyncUpdateAction.hpp"
#include "etcd/v3/AsyncHeadAction.hpp"
#include "etcd/v3/AsyncRangeAction.hpp"
#include "etcd/v3/AsyncDeleteAction.hpp"
#include "etcd/v3/AsyncPutAction.hpp"
#include "etcd/v3/AsyncWatchAction.hpp"
#include "etcd/v3/AsyncLeaseAction.hpp"
#include "etcd/v3/AsyncLockAction.hpp"
#include "etcd/v3/AsyncElectionAction.hpp"
#include "etcd/v3/AsyncTxnAction.hpp"

using grpc::Channel;

namespace etcd {
namespace detail {

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

#if defined(_WIN32)
  {
    // Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h.
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;

    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
      // Tell the user that we could not find a usable Winsock DLL.
      std::cerr << "WSAStartup failed with error: %d" << err << std::endl;
      return false;
    }
  }
#endif

  int r = getaddrinfo(target_parts[0].c_str(), target_parts[1].c_str(), &hints, &addrs);
  if (r != 0) {
    std::cerr << "warn: getaddrinfo() failed for endpoint " << target
              << " with error: " << r << std::endl;
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

const std::string strip_and_resolve_addresses(std::string const &address) {
  std::vector<std::string> addresses;
  boost::algorithm::split(addresses, address, boost::algorithm::is_any_of(",;"));
  std::string stripped_address;
  {
    std::vector<std::string> stripped_addresses;
    std::string substr("://");
    for (auto const &addr: addresses) {
      std::string::size_type idx = addr.find(substr);
      std::string target = idx == std::string::npos ? addr : addr.substr(idx + substr.length());
      etcd::detail::dns_resolve(target, stripped_addresses);
    }
    stripped_address = boost::algorithm::join(stripped_addresses, ",");
  }
  return "ipv4:///" + stripped_address;
}

const bool authenticate(std::shared_ptr<grpc::Channel> const &channel,
                        std::string const &username,
                        std::string const &password,
                        std::string &token_or_message) {
  // run a round of auth
  auto auth_stub = etcdserverpb::Auth::NewStub(channel);
  ClientContext context;
  etcdserverpb::AuthenticateRequest auth_request;
  etcdserverpb::AuthenticateResponse auth_response;
  auth_request.set_name(username);
  auth_request.set_password(password);
  auto status = auth_stub->Authenticate(&context, auth_request, &auth_response);
  if (status.ok()) {
    token_or_message = auth_response.token();
    return true;
  } else {
    token_or_message = status.error_message();
    return false;
  }
}

static std::string read_from_file(std::string const &filename) {
  std::ifstream file(filename.c_str(), std::ios::in);
  if (file.is_open()) {
    std::stringstream ss;
		ss << file.rdbuf ();
		file.close ();
		return ss.str ();
  }
  return std::string{};
}

static grpc::SslCredentialsOptions make_ssl_credentials(std::string const &ca,
                                                      std::string const &cert,
                                                      std::string const &key) {
  grpc::SslCredentialsOptions options;
  options.pem_root_certs = read_from_file(ca);
  options.pem_cert_chain = read_from_file(cert);
  options.pem_private_key = read_from_file(key);
  return options;
}

}
}

class etcd::Client::TokenAuthenticator {
  private:
    std::shared_ptr<grpc::Channel> channel_;
    std::string username_, password_, token_;
    int ttl_ = 300;  // see also --auth-token-ttl for etcd
    std::chrono::time_point<std::chrono::system_clock> updated_at;
    std::mutex mtx_;
    bool has_token_ = false;

  public:
    TokenAuthenticator(): has_token_(false) {
    }

    TokenAuthenticator(std::shared_ptr<grpc::Channel> channel,
                       std::string const &username,
                       std::string const &password,
                       const int ttl=300)
      : channel_(channel), username_(username), password_(password), ttl_(ttl), has_token_(false) {
      if ((!username.empty()) && (!(password.empty()))) {
        has_token_ = true;
        renew_if_expired(true);
      }
    }

    std::string const &renew_if_expired(const bool force = false) {
      if (!has_token_) {
        return token_;
      }
      std::lock_guard<std::mutex> scoped_lock(mtx_);
      if (force || (!token_.empty())) {
        auto tp = std::chrono::system_clock::now();
        if (force || std::chrono::duration_cast<std::chrono::seconds>(tp - updated_at).count()
          > std::max(1, ttl_ - 3)) {
          updated_at = tp;
          // auth
          if (!etcd::detail::authenticate(this->channel_, username_, password_, token_)) {
            throw std::invalid_argument("Etcd authentication failed: " + token_);
          }
        }
      }
      return token_;
    }
};

void etcd::Client::TokenAuthenticatorDeleter::operator()(etcd::Client::TokenAuthenticator *authenticator) {
  if (authenticator) {
    delete authenticator;
  }
}

struct etcd::Client::EtcdServerStubs {
  std::unique_ptr<etcdserverpb::KV::Stub> kvServiceStub;
  std::unique_ptr<etcdserverpb::Watch::Stub> watchServiceStub;
  std::unique_ptr<etcdserverpb::Lease::Stub> leaseServiceStub;
  std::unique_ptr<v3lockpb::Lock::Stub> lockServiceStub;
  std::unique_ptr<v3electionpb::Election::Stub> electionServiceStub;
};

void etcd::Client::EtcdServerStubsDeleter::operator()(etcd::Client::EtcdServerStubs *stubs) {
  if (stubs) {
    delete stubs;
  }
}

etcd::Client::Client(std::string const & address,
                     std::string const & load_balancer)
{
  // create channels
  std::string const addresses = etcd::detail::strip_and_resolve_addresses(address);
  grpc::ChannelArguments grpc_args;
  grpc_args.SetMaxSendMessageSize(std::numeric_limits<int>::max());
  grpc_args.SetMaxReceiveMessageSize(std::numeric_limits<int>::max());
  std::shared_ptr<grpc::ChannelCredentials> creds = grpc::InsecureChannelCredentials();
  grpc_args.SetLoadBalancingPolicyName(load_balancer);
  this->channel = grpc::CreateCustomChannel(addresses, creds, grpc_args);
  this->token_authenticator.reset(new TokenAuthenticator());

  // create stubs
  stubs.reset(new EtcdServerStubs{});
  stubs->kvServiceStub = KV::NewStub(this->channel);
  stubs->watchServiceStub= Watch::NewStub(this->channel);
  stubs->leaseServiceStub= Lease::NewStub(this->channel);
  stubs->lockServiceStub = Lock::NewStub(this->channel);
  stubs->electionServiceStub = Election::NewStub(this->channel);
}

etcd::Client::Client(std::string const & address,
                     grpc::ChannelArguments const & arguments)
{
  // create channels
  std::string const addresses = etcd::detail::strip_and_resolve_addresses(address);
  grpc::ChannelArguments grpc_args = arguments;
  grpc_args.SetMaxSendMessageSize(std::numeric_limits<int>::max());
  grpc_args.SetMaxReceiveMessageSize(std::numeric_limits<int>::max());
  std::shared_ptr<grpc::ChannelCredentials> creds = grpc::InsecureChannelCredentials();
  this->channel = grpc::CreateCustomChannel(addresses, creds, grpc_args);
  this->token_authenticator.reset(new TokenAuthenticator());

  // create stubs
  stubs.reset(new EtcdServerStubs{});
  stubs->kvServiceStub = KV::NewStub(this->channel);
  stubs->watchServiceStub= Watch::NewStub(this->channel);
  stubs->leaseServiceStub= Lease::NewStub(this->channel);
  stubs->lockServiceStub = Lock::NewStub(this->channel);
  stubs->electionServiceStub = Election::NewStub(this->channel);
}

etcd::Client *etcd::Client::WithUrl(std::string const & etcd_url,
                                    std::string const & load_balancer) {
  return new etcd::Client(etcd_url, load_balancer);
}

etcd::Client *etcd::Client::WithUrl(std::string const & etcd_url,
                                    grpc::ChannelArguments const & arguments) {
  return new etcd::Client(etcd_url, arguments);
}

etcd::Client::Client(std::string const & address,
                     std::string const & username,
                     std::string const & password,
                     int const auth_token_ttl,
                     std::string const & load_balancer)
{
  // create channels
  std::string const addresses = etcd::detail::strip_and_resolve_addresses(address);
  grpc::ChannelArguments grpc_args;
  grpc_args.SetMaxSendMessageSize(std::numeric_limits<int>::max());
  grpc_args.SetMaxReceiveMessageSize(std::numeric_limits<int>::max());
  std::shared_ptr<grpc::ChannelCredentials> creds = grpc::InsecureChannelCredentials();
  grpc_args.SetLoadBalancingPolicyName(load_balancer);
  this->channel = grpc::CreateCustomChannel(addresses, creds, grpc_args);

  // auth
  this->token_authenticator.reset(new TokenAuthenticator(this->channel, username, password, auth_token_ttl));

  // setup stubs
  stubs.reset(new EtcdServerStubs{});
  stubs->kvServiceStub = KV::NewStub(this->channel);
  stubs->watchServiceStub= Watch::NewStub(this->channel);
  stubs->leaseServiceStub= Lease::NewStub(this->channel);
  stubs->lockServiceStub = Lock::NewStub(this->channel);
  stubs->electionServiceStub = Election::NewStub(this->channel);
}

etcd::Client::Client(std::string const & address,
                     std::string const & username,
                     std::string const & password,
                     int const auth_token_ttl,
                     grpc::ChannelArguments const & arguments)
{
  // create channels
  std::string const addresses = etcd::detail::strip_and_resolve_addresses(address);
  grpc::ChannelArguments grpc_args = arguments;
  grpc_args.SetMaxSendMessageSize(std::numeric_limits<int>::max());
  grpc_args.SetMaxReceiveMessageSize(std::numeric_limits<int>::max());
  std::shared_ptr<grpc::ChannelCredentials> creds = grpc::InsecureChannelCredentials();
  this->channel = grpc::CreateCustomChannel(addresses, creds, grpc_args);

  // auth
  this->token_authenticator.reset(new TokenAuthenticator(this->channel, username, password, auth_token_ttl));

  // setup stubs
  stubs.reset(new EtcdServerStubs{});
  stubs->kvServiceStub = KV::NewStub(this->channel);
  stubs->watchServiceStub= Watch::NewStub(this->channel);
  stubs->leaseServiceStub= Lease::NewStub(this->channel);
  stubs->lockServiceStub = Lock::NewStub(this->channel);
  stubs->electionServiceStub = Election::NewStub(this->channel);
}

etcd::Client *etcd::Client::WithUser(std::string const & etcd_url,
                                     std::string const & username,
                                     std::string const & password,
                                     int const auth_token_ttl,
                                     std::string const & load_balancer) {
  return new etcd::Client(etcd_url, username, password, auth_token_ttl, load_balancer);
}

etcd::Client *etcd::Client::WithUser(std::string const & etcd_url,
                                     std::string const & username,
                                     std::string const & password,
                                     int const auth_token_ttl,
                                     grpc::ChannelArguments const & arguments) {
  return new etcd::Client(etcd_url, username, password, auth_token_ttl, arguments);
}


etcd::Client::Client(std::string const & address,
                     std::string const & ca,
                     std::string const & cert,
                     std::string const & key,
                     std::string const & target_name_override,
                     std::string const & load_balancer)
{
  // create channels
  std::string const addresses = etcd::detail::strip_and_resolve_addresses(address);
  grpc::ChannelArguments grpc_args;
  grpc_args.SetMaxSendMessageSize(std::numeric_limits<int>::max());
  grpc_args.SetMaxReceiveMessageSize(std::numeric_limits<int>::max());
  std::shared_ptr<grpc::ChannelCredentials> creds = grpc::SslCredentials(
      etcd::detail::make_ssl_credentials(ca, cert, key));
  grpc_args.SetLoadBalancingPolicyName(load_balancer);
  if (!target_name_override.empty()) {
    grpc_args.SetString(GRPC_SSL_TARGET_NAME_OVERRIDE_ARG, target_name_override);
  }
  this->channel = grpc::CreateCustomChannel(addresses, creds, grpc_args);
  this->token_authenticator.reset(new TokenAuthenticator());

  // setup stubs
  stubs.reset(new EtcdServerStubs{});
  stubs->kvServiceStub = KV::NewStub(this->channel);
  stubs->watchServiceStub= Watch::NewStub(this->channel);
  stubs->leaseServiceStub= Lease::NewStub(this->channel);
  stubs->lockServiceStub = Lock::NewStub(this->channel);
  stubs->electionServiceStub = Election::NewStub(this->channel);
}

etcd::Client::Client(std::string const & address,
                     std::string const & ca,
                     std::string const & cert,
                     std::string const & key,
                     std::string const & target_name_override,
                     grpc::ChannelArguments const & arguments)
{
  // create channels
  std::string const addresses = etcd::detail::strip_and_resolve_addresses(address);
  grpc::ChannelArguments grpc_args = arguments;
  grpc_args.SetMaxSendMessageSize(std::numeric_limits<int>::max());
  grpc_args.SetMaxReceiveMessageSize(std::numeric_limits<int>::max());
  std::shared_ptr<grpc::ChannelCredentials> creds = grpc::SslCredentials(
      etcd::detail::make_ssl_credentials(ca, cert, key));
  if (!target_name_override.empty()) {
    grpc_args.SetString(GRPC_SSL_TARGET_NAME_OVERRIDE_ARG, target_name_override);
  }
  this->channel = grpc::CreateCustomChannel(addresses, creds, grpc_args);
  this->token_authenticator.reset(new TokenAuthenticator());

  // setup stubs
  stubs.reset(new EtcdServerStubs{});
  stubs->kvServiceStub = KV::NewStub(this->channel);
  stubs->watchServiceStub= Watch::NewStub(this->channel);
  stubs->leaseServiceStub= Lease::NewStub(this->channel);
  stubs->lockServiceStub = Lock::NewStub(this->channel);
  stubs->electionServiceStub = Election::NewStub(this->channel);
}

etcd::Client *etcd::Client::WithSSL(std::string const & etcd_url,
                                    std::string const & ca,
                                    std::string const & cert,
                                    std::string const & key,
                                    std::string const & target_name_override,
                                    std::string const & load_balancer) {
  return new etcd::Client(etcd_url, ca, cert, key, target_name_override, load_balancer);
}

etcd::Client *etcd::Client::WithSSL(std::string const & etcd_url,
                                    grpc::ChannelArguments const & arguments,
                                    std::string const & ca,
                                    std::string const & cert,
                                    std::string const & key,
                                    std::string const & target_name_override) {
  return new etcd::Client(etcd_url, ca, cert, key, target_name_override, arguments);
}

pplx::task<etcd::Response> etcd::Client::head()
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncHeadAction> call(new etcdv3::AsyncHeadAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::get(std::string const & key)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.withPrefix = false;
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncRangeAction> call(new etcdv3::AsyncRangeAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::set(std::string const & key, std::string const & value, int ttl)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.value.assign(value);
  params.kv_stub = stubs->kvServiceStub.get();

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
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.value.assign(value);
  params.lease_id = leaseid;
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncSetAction> call(new etcdv3::AsyncSetAction(params));
  return Response::create(call);
}


pplx::task<etcd::Response> etcd::Client::add(std::string const & key, std::string const & value, int ttl)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.value.assign(value);
  params.kv_stub = stubs->kvServiceStub.get();

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
  std::shared_ptr<etcdv3::AsyncSetAction> call(new etcdv3::AsyncSetAction(params, true));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::add(std::string const & key, std::string const & value, int64_t leaseid)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.value.assign(value);
  params.lease_id = leaseid;
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncSetAction> call(new etcdv3::AsyncSetAction(params,true));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::put(std::string const & key, std::string const & value) {
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncPutAction> call(new etcdv3::AsyncPutAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::modify(std::string const & key, std::string const & value, int ttl)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.value.assign(value);
  params.kv_stub = stubs->kvServiceStub.get();

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
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.value.assign(value);
  params.lease_id = leaseid;
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncUpdateAction> call(new etcdv3::AsyncUpdateAction(params));
  return Response::create(call);
}


pplx::task<etcd::Response> etcd::Client::modify_if(std::string const & key, std::string const & value, std::string const & old_value, int ttl)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.value.assign(value);
  params.old_value.assign(old_value);
  params.kv_stub = stubs->kvServiceStub.get();

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
  std::shared_ptr<etcdv3::AsyncCompareAndSwapAction> call(
      new etcdv3::AsyncCompareAndSwapAction(params,etcdv3::AtomicityType::PREV_VALUE));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::modify_if(std::string const & key, std::string const & value, std::string const & old_value, int64_t leaseid)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.value.assign(value);
  params.old_value.assign(old_value);
  params.lease_id = leaseid;
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncCompareAndSwapAction> call(
      new etcdv3::AsyncCompareAndSwapAction(params,etcdv3::AtomicityType::PREV_VALUE));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::modify_if(std::string const & key, std::string const & value, int64_t old_index, int ttl)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.value.assign(value);
  params.old_revision = old_index;
  params.kv_stub = stubs->kvServiceStub.get();
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
  std::shared_ptr<etcdv3::AsyncCompareAndSwapAction> call(
      new etcdv3::AsyncCompareAndSwapAction(params,etcdv3::AtomicityType::PREV_INDEX));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::modify_if(std::string const & key, std::string const & value, int64_t old_index, int64_t leaseid)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.value.assign(value);
  params.lease_id = leaseid;
  params.old_revision = old_index;
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncCompareAndSwapAction> call(
      new etcdv3::AsyncCompareAndSwapAction(params,etcdv3::AtomicityType::PREV_INDEX));
  return Response::create(call);
}


pplx::task<etcd::Response> etcd::Client::rm(std::string const & key)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.withPrefix = false;
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncDeleteAction> call(new etcdv3::AsyncDeleteAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::rm_if(std::string const & key, std::string const & old_value)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.old_value.assign(old_value);
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncCompareAndDeleteAction> call(
      new etcdv3::AsyncCompareAndDeleteAction(params,etcdv3::AtomicityType::PREV_VALUE));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::rm_if(std::string const & key, int64_t old_index)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.old_revision = old_index;
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncCompareAndDeleteAction> call(
      new etcdv3::AsyncCompareAndDeleteAction(params, etcdv3::AtomicityType::PREV_INDEX));;
  return Response::create(call);

}

pplx::task<etcd::Response> etcd::Client::rmdir(std::string const & key, bool recursive)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.withPrefix = recursive;
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncDeleteAction> call(new etcdv3::AsyncDeleteAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::rmdir(std::string const & key, const char *range_end)
{
  return rmdir(key, std::string(range_end));
}

pplx::task<etcd::Response> etcd::Client::rmdir(std::string const & key, std::string const &range_end)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.range_end.assign(range_end);
  params.withPrefix = false;
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncDeleteAction> call(new etcdv3::AsyncDeleteAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::ls(std::string const & key)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.withPrefix = true;
  params.limit = 0;  // default no limit.
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncRangeAction> call(new etcdv3::AsyncRangeAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::ls(std::string const & key, size_t const limit)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.withPrefix = true;
  params.limit = limit;
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncRangeAction> call(new etcdv3::AsyncRangeAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::ls(std::string const & key, std::string const &range_end)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.range_end.assign(range_end);
  params.withPrefix = false;
  params.limit = 0;  // default no limit.
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncRangeAction> call(new etcdv3::AsyncRangeAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::ls(std::string const & key, std::string const &range_end, size_t const limit)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.range_end.assign(range_end);
  params.withPrefix = false;
  params.limit = limit;
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncRangeAction> call(new etcdv3::AsyncRangeAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::watch(std::string const & key, bool recursive)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.withPrefix = recursive;
  params.watch_stub = stubs->watchServiceStub.get();
  std::shared_ptr<etcdv3::AsyncWatchAction> call(new etcdv3::AsyncWatchAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::watch(std::string const & key, int64_t fromIndex, bool recursive)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.withPrefix = recursive;
  params.revision = fromIndex;
  params.watch_stub = stubs->watchServiceStub.get();
  std::shared_ptr<etcdv3::AsyncWatchAction> call(new etcdv3::AsyncWatchAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::watch(std::string const & key, const char *range_end)
{
  return watch(key, std::string(range_end));
}

pplx::task<etcd::Response> etcd::Client::watch(std::string const & key, std::string const & range_end)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.range_end.assign(range_end);
  params.withPrefix = false;
  params.watch_stub = stubs->watchServiceStub.get();
  std::shared_ptr<etcdv3::AsyncWatchAction> call(new etcdv3::AsyncWatchAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::watch(std::string const & key, std::string const & range_end, int64_t fromIndex)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key.assign(key);
  params.range_end.assign(range_end);
  params.withPrefix = false;
  params.revision = fromIndex;
  params.watch_stub = stubs->watchServiceStub.get();
  std::shared_ptr<etcdv3::AsyncWatchAction> call(new etcdv3::AsyncWatchAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::leasegrant(int ttl)
{
  // lease grant is special, that we are expected the callback could be invoked
  // immediately after the lease is granted by the server.
  return Response::create<etcdv3::AsyncLeaseGrantAction>([this, ttl]() {
    etcdv3::ActionParameters params;
    params.auth_token.assign(this->token_authenticator->renew_if_expired());
    params.ttl = ttl;
    params.lease_stub = stubs->leaseServiceStub.get();
    return std::make_shared<etcdv3::AsyncLeaseGrantAction>(params);
  });
}

pplx::task<std::shared_ptr<etcd::KeepAlive>> etcd::Client::leasekeepalive(int ttl) {
  return pplx::task<std::shared_ptr<etcd::KeepAlive>>([this, ttl]()
  {
    etcdv3::ActionParameters params;
    params.auth_token.assign(this->token_authenticator->renew_if_expired());
    params.ttl = ttl;
    params.lease_stub = stubs->leaseServiceStub.get();
    auto call = std::make_shared<etcdv3::AsyncLeaseGrantAction>(params);

    call->waitForResponse();
    auto v3resp = call->ParseResponse();
    return std::make_shared<KeepAlive>(*this, ttl, v3resp.get_value().kvs.lease());
  });
}

pplx::task<etcd::Response> etcd::Client::leaserevoke(int64_t lease_id)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.lease_id = lease_id;
  params.lease_stub = stubs->leaseServiceStub.get();
  std::shared_ptr<etcdv3::AsyncLeaseRevokeAction> call(new etcdv3::AsyncLeaseRevokeAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::leasetimetolive(int64_t lease_id)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.lease_id = lease_id;
  params.lease_stub = stubs->leaseServiceStub.get();
  std::shared_ptr<etcdv3::AsyncLeaseTimeToLiveAction> call(new etcdv3::AsyncLeaseTimeToLiveAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::lock(std::string const &key) {
  // routines in lock usually will be fast, less than 10 seconds.
  //
  // (base on our experiences in vineyard and GraphScope).
  static const int DEFAULT_LEASE_TTL_FOR_LOCK = 10;
  return this->lock(key, DEFAULT_LEASE_TTL_FOR_LOCK);
}

pplx::task<etcd::Response> etcd::Client::lock(std::string const &key, int lease_ttl) {
  return this->leasekeepalive(lease_ttl).then([this, key](
      pplx::task<std::shared_ptr<etcd::KeepAlive>> const& resp_task) {
    auto const &keepalive = resp_task.get();

    int64_t lease_id = keepalive->Lease();
    {
      std::lock_guard<std::mutex> lexical_scope_lock(mutex_for_keepalives);
      this->keep_alive_for_locks[lease_id] = keepalive;
    }

    etcdv3::ActionParameters params;
    params.auth_token.assign(this->token_authenticator->renew_if_expired());
    params.key = key;
    params.lease_id = lease_id;
    params.lock_stub = stubs->lockServiceStub.get();
    std::shared_ptr<etcdv3::AsyncLockAction> call(new etcdv3::AsyncLockAction(params));

    auto lock_resp = Response::create_sync(call);
    {
      std::lock_guard<std::mutex> lexical_scope_lock(mutex_for_keepalives);
      if (lock_resp.is_ok()) {
        this->leases_for_locks[lock_resp.lock_key()] = lease_id;
      } else {
        this->keep_alive_for_locks.erase(lease_id);
      }
    }
    return lock_resp;
  });
}

pplx::task<etcd::Response> etcd::Client::lock_with_lease(std::string const &key,
                                              int64_t lease_id) {
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key = key;
  params.lease_id = lease_id;
  params.lock_stub = stubs->lockServiceStub.get();
  std::shared_ptr<etcdv3::AsyncLockAction> call(new etcdv3::AsyncLockAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::unlock(std::string const &lock_key) {
  // issue a "unlock" request
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.key = lock_key;
  params.lock_stub = stubs->lockServiceStub.get();
  std::shared_ptr<etcdv3::AsyncUnlockAction> call(new etcdv3::AsyncUnlockAction(params));

  // cancel the KeepAlive first, if it exists
  {
    std::lock_guard<std::mutex> lexical_scope_lock(mutex_for_keepalives);
    auto p_leases = this->leases_for_locks.find(lock_key);
    if (p_leases != this->leases_for_locks.end()) {
      auto p_keeps_alive = this->keep_alive_for_locks.find(p_leases->second);
      if (p_keeps_alive != this->keep_alive_for_locks.end()) {
        this->keep_alive_for_locks.erase(p_keeps_alive);
      } else {
#if !defined(NDEBUG)
        std::cerr << "Keepalive for lease not found" << std::endl;
#endif
      }
      this->leases_for_locks.erase(p_leases);
    } else {
#if !defined(NDEBUG)
      std::cerr << "Lease for lock not found" << std::endl;
#endif
    }
  }

  // wait in the io_context loop.
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::txn(etcdv3::Transaction const &txn) {
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.kv_stub = stubs->kvServiceStub.get();
  std::shared_ptr<etcdv3::AsyncTxnAction> call(new etcdv3::AsyncTxnAction(params, txn));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::campaign(
    std::string const &name, int64_t lease_id, std::string const &value) {
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.name = name;
  params.lease_id = lease_id;
  params.value = value;
  params.election_stub = stubs->electionServiceStub.get();
  std::shared_ptr<etcdv3::AsyncCampaignAction> call(new etcdv3::AsyncCampaignAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::proclaim(
    std::string const &name, int64_t lease_id,
    std::string const &key, int64_t revision, std::string const &value) {
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.name = name;
  params.lease_id = lease_id;
  params.key = key;
  params.revision = revision;
  params.value = value;
  params.election_stub = stubs->electionServiceStub.get();
  std::shared_ptr<etcdv3::AsyncProclaimAction> call(new etcdv3::AsyncProclaimAction(params));
  return Response::create(call);
}

pplx::task<etcd::Response> etcd::Client::leader(std::string const &name) {
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.name = name;
  params.election_stub = stubs->electionServiceStub.get();
  std::shared_ptr<etcdv3::AsyncLeaderAction> call(new etcdv3::AsyncLeaderAction(params));
  return Response::create(call);
}

std::unique_ptr<etcd::Client::Observer> etcd::Client::observe(
    std::string const &name, const bool once) {
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.name.assign(name);
  params.election_stub = stubs->electionServiceStub.get();
  std::shared_ptr<etcdv3::AsyncObserveAction> call(new etcdv3::AsyncObserveAction(params, once));
  std::unique_ptr<Observer> observer(new Observer());
  observer->action = call;
  observer->resp = Response::create(call);
  return observer;
}

std::unique_ptr<etcd::Client::Observer> etcd::Client::observe(
    std::string const &name, std::function<void(Response)> callback, const bool once) {
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.name.assign(name);
  params.election_stub = stubs->electionServiceStub.get();
  std::shared_ptr<etcdv3::AsyncObserveAction> call(new etcdv3::AsyncObserveAction(params, once));
  std::unique_ptr<Observer> observer(new Observer());
  observer->action = call;
  observer->resp = Response::create(call, callback);
  return observer;
}

pplx::task<etcd::Response> etcd::Client::resign(
    std::string const &name, int64_t lease_id, std::string const &key, int64_t revision) {
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.name = name;
  params.lease_id = lease_id;
  params.key = key;
  params.revision = revision;
  params.election_stub = stubs->electionServiceStub.get();
  std::shared_ptr<etcdv3::AsyncResignAction> call(new etcdv3::AsyncResignAction(params));
  return Response::create(call);
}

const std::string &etcd::Client::current_auth_token() const {
  return this->token_authenticator->renew_if_expired();
}

etcd::Client::Observer::~Observer() {
  if (action != nullptr) {
    action->CancelObserve();
    resp.wait();
  }
}
