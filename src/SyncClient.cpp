#if defined(_WIN32)
// see also:
// https://stackoverflow.com/questions/2561368/illegal-token-on-right-side-of
#define NOMINMAX
#endif

#ifdef __ANDROID__
#include <netinet/in.h>
#endif

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <cstddef>
#include <ratio>

#include "etcd/Value.hpp"

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#endif

#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>

#include <grpc++/grpc++.h>
#include <grpc++/security/credentials.h>
#include <grpc++/support/status_code_enum.h>
#include <grpc/grpc.h>  // for grpc_lame_client_channel_create()

#include "proto/rpc.grpc.pb.h"
#include "proto/v3election.grpc.pb.h"
#include "proto/v3lock.grpc.pb.h"

#include "etcd/KeepAlive.hpp"
#include "etcd/SyncClient.hpp"
#include "etcd/v3/Action.hpp"
#include "etcd/v3/AsyncGRPC.hpp"
#include "etcd/v3/Transaction.hpp"
#include "etcd/v3/action_constants.hpp"

namespace grpc {
// forward declaration for compatibility with older grpc versions
std::shared_ptr<Channel> CreateChannelInternal(
    const std::string& host, grpc_channel* c_channel,
#if defined(WITH_GRPC_CREATE_CHANNEL_INTERNAL_UNIQUE_POINTER)
    std::unique_ptr<std::vector<
        std::unique_ptr<experimental::ClientInterceptorFactoryInterface>>>
        interceptor_creators
#else
    std::vector<
        std::unique_ptr<grpc::experimental::ClientInterceptorFactoryInterface>>
        interceptor_creators
#endif
);
}  // namespace grpc

namespace etcd {
namespace detail {

static void string_split(std::vector<std::string>& dests,
                         std::string const& src, std::string const& seps) {
  dests.clear();
  std::string::const_iterator start = src.begin();
  std::string::const_iterator end = src.end();
  std::string::const_iterator next =
      std::find_first_of(start, end, seps.begin(), seps.end());
  while (next != end) {
    dests.push_back(std::string(start, next));
    start = next + 1;
    next = std::find_first_of(start, end, seps.begin(), seps.end());
  }
  if (start != end) {
    dests.push_back(std::string(start, end));
  }
}

static std::string string_join(std::vector<std::string> const& srcs,
                               std::string const& sep) {
  std::stringstream ss;
  if (!srcs.empty()) {
    ss << srcs[0];
    for (size_t i = 1; i < srcs.size(); ++i) {
      ss << sep << srcs[i];
    }
  }
  return ss.str();
}

static bool dns_resolve(std::string const& target,
                        std::vector<std::string>& endpoints, bool ipv4 = true) {
  std::vector<std::string> target_parts;
  bool ipv6_url{false};
  {
    size_t rindex = target.rfind(':');
    if (rindex == target.npos) {
#ifndef NDEBUG
      std::cerr << "[warn] invalid URL: " << target << ", expects 'host:port'"
                << std::endl;
#endif
      return false;
    }

    std::string host(target.substr(0, rindex));

    // host format is [ipv6]
    if (!ipv4 && !host.empty() && host[0] == '[' &&
        host[host.size() - 1] == ']') {
      host = target.substr(1, rindex - 2);
      ipv6_url = true;
    }

    target_parts.push_back(host);
    target_parts.push_back(target.substr(rindex + 1));
  }

#if defined(_WIN32)
  {
    // Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h.
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;

    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
      // Tell the user that we could not find a usable Winsock DLL.
#ifndef NDEBUG
      std::cerr << "[warn] WSAStartup failed with error: %d" << err
                << std::endl;
#endif
      return false;
    }
  }
#endif

  if (ipv6_url) {
    // check valid ipv6
    struct sockaddr_in6 sa6;
    if (inet_pton(AF_INET6, target_parts[0].c_str(), &(sa6.sin6_addr)) == 1) {
      endpoints.emplace_back(target);
      return true;
    }
    return false;
  }

  struct addrinfo hints = {}, *addrs;
  hints.ai_family = ipv4 ? AF_INET : AF_INET6;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  int r = getaddrinfo(target_parts[0].c_str(), target_parts[1].c_str(), &hints,
                      &addrs);
  if (r != 0) {
#ifndef NDEBUG
    std::cerr << "[warn] getaddrinfo() as " << (ipv4 ? "ipv4" : "ipv6")
              << " failed for endpoint " << target << " with error: " << r
              << ", " << strerror(errno) << std::endl;
#endif
    return false;
  }

  char host[16] = {'\0'};
  for (struct addrinfo* addr = addrs; addr != nullptr; addr = addr->ai_next) {
    if (addr->ai_family != AF_INET && addr->ai_family != AF_INET6) {
      continue;
    }
    memset(host, '\0', sizeof(host));
    int r = getnameinfo(addr->ai_addr, addr->ai_addrlen, host, sizeof(host),
                        NULL, 0, NI_NUMERICHOST);
    if (r != 0) {
#ifndef NDEBUG
      std::cerr << "[warn] getnameinfo() failed for endpoint " << target
                << " with error: " << r << ", " << strerror(errno) << std::endl;
#endif
      continue;
    }
    std::string host_string = host;
    if (addr->ai_family == AF_INET6) {
      host_string = "[" + host_string + "]";
    }
    endpoints.emplace_back(host_string + ":" + target_parts[1]);
  }
  freeaddrinfo(addrs);
  return true;
}

const std::string strip_and_resolve_addresses(std::string const& address) {
  std::vector<std::string> addresses;
  string_split(addresses, address, ",;");
  std::string stripped_v4_address, stripped_v6_address;
  {
    std::vector<std::string> stripped_v4_addresses, stripped_v6_addresses;
    std::string substr("://");
    for (auto const& addr : addresses) {
      std::string::size_type idx = addr.find(substr);
      std::string target =
          idx == std::string::npos ? addr : addr.substr(idx + substr.length());
      etcd::detail::dns_resolve(target, stripped_v4_addresses, true);
      etcd::detail::dns_resolve(target, stripped_v6_addresses, false);
    }
    stripped_v4_address = string_join(stripped_v4_addresses, ",");
    stripped_v6_address = string_join(stripped_v6_addresses, ",");
  }
  // prefer resolved ipv4 addresses
  if (!stripped_v4_address.empty()) {
    return "ipv4:///" + stripped_v4_address;
  }
  if (!stripped_v6_address.empty()) {
    return "ipv6:///" + stripped_v6_address;
  }
  return std::string{};
}

bool authenticate(std::shared_ptr<grpc::Channel> const& channel,
                  std::string const& username, std::string const& password,
                  std::string& token_or_message) {
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

static std::string read_from_file(std::string const& filename) {
  std::ifstream file(filename.c_str(), std::ios::in);
  if (file.is_open()) {
    std::stringstream ss;
    ss << file.rdbuf();
    file.close();
    return ss.str();
  } else {
#ifndef NDEBUG
    std::cerr << "[error] failed to load given file '" << filename << "', "
              << strerror(errno) << std::endl;
#endif
    return std::string{};
  }
}

static grpc::SslCredentialsOptions make_ssl_credentials(
    std::string const& ca, std::string const& cert, std::string const& key) {
  grpc::SslCredentialsOptions options;
  options.pem_root_certs = read_from_file(ca);
  options.pem_cert_chain = read_from_file(cert);
  options.pem_private_key = read_from_file(key);
  return options;
}

template <typename T, typename... Args>
std::unique_ptr<T> make_unique_ptr(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

static std::shared_ptr<grpc::Channel> create_grpc_channel(
    const std::string& address,
    const std::shared_ptr<grpc::ChannelCredentials> creds,
    const grpc::ChannelArguments& grpc_args) {
  const std::string addresses =
      etcd::detail::strip_and_resolve_addresses(address);
#ifndef NDEBUG
  std::cerr << "[debug] resolved addresses: " << addresses << std::endl;
#endif
  if (addresses.empty() || addresses == "ipv4:///" || addresses == "ipv6:///") {
    // bypass grpc initialization to avoid noisy logs from grpc
    return grpc::CreateChannelInternal(
        "",
        grpc_lame_client_channel_create(addresses.c_str(), GRPC_STATUS_INTERNAL,
                                        "the target uri is not valid"),
#if defined(WITH_GRPC_CREATE_CHANNEL_INTERNAL_UNIQUE_POINTER)
        nullptr
#else
        std::vector<std::unique_ptr<
            grpc::experimental::ClientInterceptorFactoryInterface>>()
#endif
    );
  } else {
    return grpc::CreateCustomChannel(addresses, creds, grpc_args);
  }
}

}  // namespace detail
}  // namespace etcd

class etcd::SyncClient::TokenAuthenticator {
 private:
  std::shared_ptr<grpc::Channel> channel_;
  std::string username_, password_, token_;
  int ttl_ = 300;  // see also --auth-token-ttl for etcd
  std::chrono::time_point<std::chrono::system_clock> updated_at;
  std::mutex mtx_;
  bool has_token_ = false;

 public:
  TokenAuthenticator() : has_token_(false) {}

  TokenAuthenticator(std::shared_ptr<grpc::Channel> channel,
                     std::string const& username, std::string const& password,
                     const int ttl = 300)
      : channel_(channel),
        username_(username),
        password_(password),
        ttl_(ttl),
        has_token_(false) {
    if ((!username.empty()) && (!(password.empty()))) {
      has_token_ = true;
      renew_if_expired(true);
    }
  }

  std::string const& renew_if_expired(const bool force = false) {
    if (!has_token_) {
      return token_;
    }
    std::lock_guard<std::mutex> scoped_lock(mtx_);
    if (force || (!token_.empty())) {
      auto tp = std::chrono::system_clock::now();
      if (force ||
          std::chrono::duration_cast<std::chrono::seconds>(tp - updated_at)
                  .count() > std::max(1, ttl_ - 3)) {
        updated_at = tp;
        // auth
        if (!etcd::detail::authenticate(this->channel_, username_, password_,
                                        token_)) {
          // n.b.: no throw here as the failure of auth will be propagated
          // to client when it is asked to issue requests.
          //
          // throw std::invalid_argument("Etcd authentication failed: " +
          // token_);
        }
      }
    }
    return token_;
  }
};

void etcd::SyncClient::TokenAuthenticatorDeleter::operator()(
    etcd::SyncClient::TokenAuthenticator* authenticator) {
  if (authenticator) {
    delete authenticator;
  }
}

struct etcd::SyncClient::EtcdServerStubs {
  std::unique_ptr<etcdserverpb::KV::Stub> kvServiceStub;
  std::unique_ptr<etcdserverpb::Watch::Stub> watchServiceStub;
  std::unique_ptr<etcdserverpb::Cluster::Stub> clusterServiceStub;
  std::unique_ptr<etcdserverpb::Lease::Stub> leaseServiceStub;
  std::unique_ptr<v3lockpb::Lock::Stub> lockServiceStub;
  std::unique_ptr<v3electionpb::Election::Stub> electionServiceStub;
};

void etcd::SyncClient::EtcdServerStubsDeleter::operator()(
    etcd::SyncClient::EtcdServerStubs* stubs) {
  if (stubs) {
    delete stubs;
  }
}

etcd::SyncClient::SyncClient(std::string const& address,
                             std::string const& load_balancer) {
  // create channels
  grpc::ChannelArguments grpc_args;
  grpc_args.SetMaxSendMessageSize(std::numeric_limits<int>::max());
  grpc_args.SetMaxReceiveMessageSize(std::numeric_limits<int>::max());
  std::shared_ptr<grpc::ChannelCredentials> creds =
      grpc::InsecureChannelCredentials();
  grpc_args.SetLoadBalancingPolicyName(load_balancer);
  this->channel = etcd::detail::create_grpc_channel(address, creds, grpc_args);
  this->token_authenticator.reset(new TokenAuthenticator());

  // create stubs
  stubs.reset(new EtcdServerStubs{});
  stubs->kvServiceStub = KV::NewStub(this->channel);
  stubs->watchServiceStub = Watch::NewStub(this->channel);
  stubs->clusterServiceStub = Cluster::NewStub(this->channel);
  stubs->leaseServiceStub = Lease::NewStub(this->channel);
  stubs->lockServiceStub = Lock::NewStub(this->channel);
  stubs->electionServiceStub = Election::NewStub(this->channel);
}

etcd::SyncClient::SyncClient(std::string const& address,
                             grpc::ChannelArguments const& arguments) {
  // create channels
  grpc::ChannelArguments grpc_args = arguments;
  grpc_args.SetMaxSendMessageSize(std::numeric_limits<int>::max());
  grpc_args.SetMaxReceiveMessageSize(std::numeric_limits<int>::max());
  std::shared_ptr<grpc::ChannelCredentials> creds =
      grpc::InsecureChannelCredentials();
  this->channel = etcd::detail::create_grpc_channel(address, creds, grpc_args);
  this->token_authenticator.reset(new TokenAuthenticator());

  // create stubs
  stubs.reset(new EtcdServerStubs{});
  stubs->kvServiceStub = KV::NewStub(this->channel);
  stubs->watchServiceStub = Watch::NewStub(this->channel);
  stubs->leaseServiceStub = Lease::NewStub(this->channel);
  stubs->lockServiceStub = Lock::NewStub(this->channel);
  stubs->electionServiceStub = Election::NewStub(this->channel);
}

etcd::SyncClient* etcd::SyncClient::WithUrl(std::string const& etcd_url,
                                            std::string const& load_balancer) {
  return new etcd::SyncClient(etcd_url, load_balancer);
}

etcd::SyncClient* etcd::SyncClient::WithUrl(
    std::string const& etcd_url, grpc::ChannelArguments const& arguments) {
  return new etcd::SyncClient(etcd_url, arguments);
}

etcd::SyncClient::SyncClient(std::string const& address,
                             std::string const& username,
                             std::string const& password,
                             int const auth_token_ttl,
                             std::string const& load_balancer) {
  // create channels
  grpc::ChannelArguments grpc_args;
  grpc_args.SetMaxSendMessageSize(std::numeric_limits<int>::max());
  grpc_args.SetMaxReceiveMessageSize(std::numeric_limits<int>::max());
  std::shared_ptr<grpc::ChannelCredentials> creds =
      grpc::InsecureChannelCredentials();
  grpc_args.SetLoadBalancingPolicyName(load_balancer);
  this->channel = etcd::detail::create_grpc_channel(address, creds, grpc_args);

  // auth
  this->token_authenticator.reset(new TokenAuthenticator(
      this->channel, username, password, auth_token_ttl));

  // setup stubs
  stubs.reset(new EtcdServerStubs{});
  stubs->kvServiceStub = KV::NewStub(this->channel);
  stubs->watchServiceStub = Watch::NewStub(this->channel);
  stubs->leaseServiceStub = Lease::NewStub(this->channel);
  stubs->lockServiceStub = Lock::NewStub(this->channel);
  stubs->electionServiceStub = Election::NewStub(this->channel);
}

etcd::SyncClient::SyncClient(std::string const& address,
                             std::string const& username,
                             std::string const& password,
                             int const auth_token_ttl,
                             grpc::ChannelArguments const& arguments) {
  // create channels
  grpc::ChannelArguments grpc_args = arguments;
  grpc_args.SetMaxSendMessageSize(std::numeric_limits<int>::max());
  grpc_args.SetMaxReceiveMessageSize(std::numeric_limits<int>::max());
  std::shared_ptr<grpc::ChannelCredentials> creds =
      grpc::InsecureChannelCredentials();
  this->channel = etcd::detail::create_grpc_channel(address, creds, grpc_args);

  // auth
  this->token_authenticator.reset(new TokenAuthenticator(
      this->channel, username, password, auth_token_ttl));

  // setup stubs
  stubs.reset(new EtcdServerStubs{});
  stubs->kvServiceStub = KV::NewStub(this->channel);
  stubs->watchServiceStub = Watch::NewStub(this->channel);
  stubs->leaseServiceStub = Lease::NewStub(this->channel);
  stubs->lockServiceStub = Lock::NewStub(this->channel);
  stubs->electionServiceStub = Election::NewStub(this->channel);
}

etcd::SyncClient* etcd::SyncClient::WithUser(std::string const& etcd_url,
                                             std::string const& username,
                                             std::string const& password,
                                             int const auth_token_ttl,
                                             std::string const& load_balancer) {
  return new etcd::SyncClient(etcd_url, username, password, auth_token_ttl,
                              load_balancer);
}

etcd::SyncClient* etcd::SyncClient::WithUser(
    std::string const& etcd_url, std::string const& username,
    std::string const& password, int const auth_token_ttl,
    grpc::ChannelArguments const& arguments) {
  return new etcd::SyncClient(etcd_url, username, password, auth_token_ttl,
                              arguments);
}

etcd::SyncClient::SyncClient(std::string const& address, std::string const& ca,
                             std::string const& cert,
                             std::string const& privkey,
                             std::string const& target_name_override,
                             std::string const& load_balancer) {
  // create channels
  grpc::ChannelArguments grpc_args;
  grpc_args.SetMaxSendMessageSize(std::numeric_limits<int>::max());
  grpc_args.SetMaxReceiveMessageSize(std::numeric_limits<int>::max());
  std::shared_ptr<grpc::ChannelCredentials> creds = grpc::SslCredentials(
      etcd::detail::make_ssl_credentials(ca, cert, privkey));
  grpc_args.SetLoadBalancingPolicyName(load_balancer);
  if (!target_name_override.empty()) {
    grpc_args.SetString(GRPC_SSL_TARGET_NAME_OVERRIDE_ARG,
                        target_name_override);
  }
  this->channel = etcd::detail::create_grpc_channel(address, creds, grpc_args);
  this->token_authenticator.reset(new TokenAuthenticator());

  // setup stubs
  stubs.reset(new EtcdServerStubs{});
  stubs->kvServiceStub = KV::NewStub(this->channel);
  stubs->watchServiceStub = Watch::NewStub(this->channel);
  stubs->leaseServiceStub = Lease::NewStub(this->channel);
  stubs->lockServiceStub = Lock::NewStub(this->channel);
  stubs->electionServiceStub = Election::NewStub(this->channel);
}

etcd::SyncClient::SyncClient(std::string const& address, std::string const& ca,
                             std::string const& cert,
                             std::string const& privkey,
                             std::string const& target_name_override,
                             grpc::ChannelArguments const& arguments) {
  // create channels
  grpc::ChannelArguments grpc_args = arguments;
  grpc_args.SetMaxSendMessageSize(std::numeric_limits<int>::max());
  grpc_args.SetMaxReceiveMessageSize(std::numeric_limits<int>::max());
  std::shared_ptr<grpc::ChannelCredentials> creds = grpc::SslCredentials(
      etcd::detail::make_ssl_credentials(ca, cert, privkey));
  if (!target_name_override.empty()) {
    grpc_args.SetString(GRPC_SSL_TARGET_NAME_OVERRIDE_ARG,
                        target_name_override);
  }
  this->channel = etcd::detail::create_grpc_channel(address, creds, grpc_args);
  this->token_authenticator.reset(new TokenAuthenticator());

  // setup stubs
  stubs.reset(new EtcdServerStubs{});
  stubs->kvServiceStub = KV::NewStub(this->channel);
  stubs->watchServiceStub = Watch::NewStub(this->channel);
  stubs->leaseServiceStub = Lease::NewStub(this->channel);
  stubs->lockServiceStub = Lock::NewStub(this->channel);
  stubs->electionServiceStub = Election::NewStub(this->channel);
}

etcd::SyncClient* etcd::SyncClient::WithSSL(
    std::string const& etcd_url, std::string const& ca, std::string const& cert,
    std::string const& privkey, std::string const& target_name_override,
    std::string const& load_balancer) {
  return new etcd::SyncClient(etcd_url, ca, cert, privkey, target_name_override,
                              load_balancer);
}

etcd::SyncClient* etcd::SyncClient::WithSSL(
    std::string const& etcd_url, grpc::ChannelArguments const& arguments,
    std::string const& ca, std::string const& cert, std::string const& privkey,
    std::string const& target_name_override) {
  return new etcd::SyncClient(etcd_url, ca, cert, privkey, target_name_override,
                              arguments);
}

etcd::SyncClient::~SyncClient() {
  stubs.reset();
  channel.reset();
}

etcd::Response etcd::SyncClient::head() {
  return Response::create(this->head_internal());
}

std::shared_ptr<etcdv3::AsyncHeadAction> etcd::SyncClient::head_internal() {
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.kv_stub = stubs->kvServiceStub.get();
  return std::make_shared<etcdv3::AsyncHeadAction>(std::move(params));
}

etcd::Response etcd::SyncClient::get(std::string const& key) {
  return Response::create(this->get_internal(key));
}

etcd::Response etcd::SyncClient::get(std::string const& key,
                                     const int64_t revision) {
  return Response::create(this->get_internal(key, revision));
}

std::shared_ptr<etcdv3::AsyncRangeAction> etcd::SyncClient::get_internal(
    std::string const& key, int64_t revision) {
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.revision = revision;
  params.withPrefix = false;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.kv_stub = stubs->kvServiceStub.get();
  return std::make_shared<etcdv3::AsyncRangeAction>(std::move(params));
}

etcd::Response etcd::SyncClient::set(std::string const& key,
                                     std::string const& value,
                                     const int64_t leaseid) {
  return Response::create(this->put_internal(key, value, leaseid));
}

etcd::Response etcd::SyncClient::add(std::string const& key,
                                     std::string const& value,
                                     const int64_t leaseid) {
  return Response::create(this->add_internal(key, value, leaseid));
}

std::shared_ptr<etcdv3::AsyncSetAction> etcd::SyncClient::add_internal(
    std::string const& key, std::string const& value, const int64_t leaseid) {
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.lease_id = leaseid;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.kv_stub = stubs->kvServiceStub.get();
  return std::make_shared<etcdv3::AsyncSetAction>(std::move(params), true);
}

etcd::Response etcd::SyncClient::put(std::string const& key,
                                     std::string const& value) {
  return Response::create(this->put_internal(key, value));
}

etcd::Response etcd::SyncClient::put(std::string const& key,
                                     std::string const& value,
                                     const int64_t leaseId) {
  return Response::create(this->put_internal(key, value, leaseId));
}

std::shared_ptr<etcdv3::AsyncPutAction> etcd::SyncClient::put_internal(
    std::string const& key, std::string const& value, const int64_t leaseId) {
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.lease_id = leaseId;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.kv_stub = stubs->kvServiceStub.get();
  return std::make_shared<etcdv3::AsyncPutAction>(std::move(params));
}

etcd::Response etcd::SyncClient::modify(std::string const& key,
                                        std::string const& value,
                                        const int64_t leaseid) {
  return Response::create(this->modify_internal(key, value, leaseid));
}

std::shared_ptr<etcdv3::AsyncUpdateAction> etcd::SyncClient::modify_internal(
    std::string const& key, std::string const& value, const int64_t leaseid) {
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.lease_id = leaseid;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.kv_stub = stubs->kvServiceStub.get();
  return std::make_shared<etcdv3::AsyncUpdateAction>(std::move(params));
}

etcd::Response etcd::SyncClient::modify_if(std::string const& key,
                                           std::string const& value,
                                           std::string const& old_value,
                                           const int64_t leaseid) {
  return Response::create(this->modify_if_internal(
      key, value, 0, old_value, etcdv3::AtomicityType::PREV_VALUE, leaseid));
}

etcd::Response etcd::SyncClient::modify_if(std::string const& key,
                                           std::string const& value,
                                           int64_t old_index, int64_t leaseid) {
  return Response::create(this->modify_if_internal(
      key, value, old_index, "", etcdv3::AtomicityType::PREV_INDEX, leaseid));
}

std::shared_ptr<etcdv3::AsyncCompareAndSwapAction>
etcd::SyncClient::modify_if_internal(
    std::string const& key, std::string const& value, int64_t old_index,
    std::string const& old_value, etcdv3::AtomicityType const& atomicity_type,
    const int64_t leaseId) {
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.value.assign(value);
  params.lease_id = leaseId;
  params.old_revision = old_index;
  params.old_value = old_value;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.kv_stub = stubs->kvServiceStub.get();
  return std::make_shared<etcdv3::AsyncCompareAndSwapAction>(std::move(params),
                                                             atomicity_type);
}

etcd::Response etcd::SyncClient::rm(std::string const& key) {
  return Response::create(this->rm_internal(key));
}

std::shared_ptr<etcdv3::AsyncDeleteAction> etcd::SyncClient::rm_internal(
    std::string const& key) {
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.withPrefix = false;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.kv_stub = stubs->kvServiceStub.get();
  return std::make_shared<etcdv3::AsyncDeleteAction>(std::move(params));
}

etcd::Response etcd::SyncClient::rm_if(std::string const& key,
                                       std::string const& old_value) {
  return Response::create(this->rm_if_internal(
      key, 0, old_value, etcdv3::AtomicityType::PREV_VALUE));
}

etcd::Response etcd::SyncClient::rm_if(std::string const& key,
                                       int64_t old_index) {
  return Response::create(this->rm_if_internal(
      key, old_index, "", etcdv3::AtomicityType::PREV_INDEX));
}

std::shared_ptr<etcdv3::AsyncCompareAndDeleteAction>
etcd::SyncClient::rm_if_internal(std::string const& key, int64_t old_index,
                                 const std::string& old_value,
                                 etcdv3::AtomicityType const& atomicity_type) {
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.withPrefix = false;
  params.old_revision = old_index;
  params.old_value = old_value;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.kv_stub = stubs->kvServiceStub.get();
  return std::make_shared<etcdv3::AsyncCompareAndDeleteAction>(
      std::move(params), atomicity_type);
}

etcd::Response etcd::SyncClient::rmdir(std::string const& key, bool recursive) {
  return Response::create(this->rmdir_internal(key, recursive));
}

std::shared_ptr<etcdv3::AsyncDeleteAction> etcd::SyncClient::rmdir_internal(
    std::string const& key, bool recursive) {
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.withPrefix = recursive;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.kv_stub = stubs->kvServiceStub.get();
  return std::make_shared<etcdv3::AsyncDeleteAction>(std::move(params));
}

etcd::Response etcd::SyncClient::rmdir(std::string const& key,
                                       const char* range_end) {
  return rmdir(key, std::string(range_end));
}

etcd::Response etcd::SyncClient::rmdir(std::string const& key,
                                       std::string const& range_end) {
  return Response::create(this->rmdir_internal(key, range_end));
}

std::shared_ptr<etcdv3::AsyncDeleteAction> etcd::SyncClient::rmdir_internal(
    std::string const& key, std::string const& range_end) {
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.range_end.assign(range_end);
  params.withPrefix = false;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.kv_stub = stubs->kvServiceStub.get();
  return std::make_shared<etcdv3::AsyncDeleteAction>(std::move(params));
}

etcd::Response etcd::SyncClient::ls(std::string const& key) {
  return Response::create(this->ls_internal(key, 0 /* default: no limit */));
}

etcd::Response etcd::SyncClient::ls(std::string const& key,
                                    size_t const limit) {
  return Response::create(this->ls_internal(key, limit));
}

etcd::Response etcd::SyncClient::ls(std::string const& key, size_t const limit,
                                    int64_t revision) {
  return Response::create(this->ls_internal(key, limit, false, revision));
}

etcd::Response etcd::SyncClient::ls(std::string const& key,
                                    std::string const& range_end) {
  return Response::create(
      this->ls_internal(key, range_end, 0 /* default: no limit */));
}

etcd::Response etcd::SyncClient::ls(std::string const& key,
                                    std::string const& range_end,
                                    size_t const limit) {
  return Response::create(this->ls_internal(key, range_end, limit));
}

etcd::Response etcd::SyncClient::ls(std::string const& key,
                                    std::string const& range_end,
                                    size_t const limit, int64_t revision) {
  return Response::create(
      this->ls_internal(key, range_end, limit, false, revision));
}

etcd::Response etcd::SyncClient::keys(std::string const& key) {
  return Response::create(
      this->ls_internal(key, 0 /* default: no limit */, true));
}

etcd::Response etcd::SyncClient::keys(std::string const& key,
                                      size_t const limit) {
  return Response::create(this->ls_internal(key, limit, true));
}

etcd::Response etcd::SyncClient::keys(std::string const& key,
                                      size_t const limit, int64_t revision) {
  return Response::create(this->ls_internal(key, limit, true, revision));
}

etcd::Response etcd::SyncClient::keys(std::string const& key,
                                      std::string const& range_end) {
  return Response::create(
      this->ls_internal(key, range_end, 0 /* default: no limit */, true));
}

etcd::Response etcd::SyncClient::keys(std::string const& key,
                                      std::string const& range_end,
                                      size_t const limit) {
  return Response::create(this->ls_internal(key, range_end, limit, true));
}

etcd::Response etcd::SyncClient::keys(std::string const& key,
                                      std::string const& range_end,
                                      size_t const limit, int64_t revision) {
  return Response::create(
      this->ls_internal(key, range_end, limit, true, revision));
}

std::shared_ptr<etcdv3::AsyncRangeAction> etcd::SyncClient::ls_internal(
    std::string const& key, size_t const limit, bool const keys_only,
    int64_t revision) {
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.keys_only = keys_only;
  params.withPrefix = true;
  params.limit = limit;
  params.revision = revision;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.kv_stub = stubs->kvServiceStub.get();
  return std::make_shared<etcdv3::AsyncRangeAction>(std::move(params));
}

std::shared_ptr<etcdv3::AsyncRangeAction> etcd::SyncClient::ls_internal(
    std::string const& key, std::string const& range_end, size_t const limit,
    bool const keys_only, int64_t revision) {
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.range_end.assign(range_end);
  params.keys_only = keys_only;
  params.withPrefix = false;
  params.limit = limit;
  params.revision = revision;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.kv_stub = stubs->kvServiceStub.get();
  return std::make_shared<etcdv3::AsyncRangeAction>(std::move(params));
}

etcd::Response etcd::SyncClient::watch(std::string const& key, bool recursive) {
  return Response::create(
      this->watch_internal(key, 0 /* from current location */, recursive));
}

etcd::Response etcd::SyncClient::watch(std::string const& key,
                                       int64_t fromIndex, bool recursive) {
  return Response::create(this->watch_internal(key, fromIndex, recursive));
}

std::shared_ptr<etcdv3::AsyncWatchAction> etcd::SyncClient::watch_internal(
    std::string const& key, int64_t fromIndex, bool recursive) {
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.withPrefix = recursive;
  params.revision = fromIndex;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.watch_stub = stubs->watchServiceStub.get();
  return std::make_shared<etcdv3::AsyncWatchAction>(std::move(params));
}

etcd::Response etcd::SyncClient::watch(std::string const& key,
                                       const char* range_end) {
  return watch(key, std::string(range_end));
}

etcd::Response etcd::SyncClient::watch(std::string const& key,
                                       std::string const& range_end) {
  return Response::create(
      this->watch_internal(key, range_end, 0 /* from current location */));
}

etcd::Response etcd::SyncClient::watch(std::string const& key,
                                       std::string const& range_end,
                                       int64_t fromIndex) {
  return Response::create(this->watch_internal(key, range_end, fromIndex));
}

std::shared_ptr<etcdv3::AsyncWatchAction> etcd::SyncClient::watch_internal(
    std::string const& key, std::string const& range_end, int64_t fromIndex) {
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.range_end.assign(range_end);
  params.withPrefix = false;
  params.revision = fromIndex;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.watch_stub = stubs->watchServiceStub.get();
  return std::make_shared<etcdv3::AsyncWatchAction>(std::move(params));
}

etcd::Response etcd::SyncClient::leasegrant(int ttl) {
  // lease grant is special, that we are expected the callback could be invoked
  // immediately after the lease is granted by the server.
  //
  // otherwise when we get the response, the lease might already has expired.
  return Response::create<etcdv3::AsyncLeaseGrantAction>([this, ttl]() {
    etcdv3::ActionParameters params;
    params.auth_token.assign(this->token_authenticator->renew_if_expired());
    params.grpc_timeout = this->grpc_timeout;
    params.lease_stub = stubs->leaseServiceStub.get();
    params.ttl = ttl;
    return std::make_shared<etcdv3::AsyncLeaseGrantAction>(std::move(params));
  });
}

std::shared_ptr<etcd::KeepAlive> etcd::SyncClient::leasekeepalive(int ttl) {
  etcdv3::ActionParameters params;
  params.ttl = ttl;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.lease_stub = stubs->leaseServiceStub.get();

  // keep alive is synchronous in two folds:
  //
  // - the wait-for-response starts immediately after the request been issued
  // - the keep alive thread starts immediately after the lease been granted
  auto call =
      std::make_shared<etcdv3::AsyncLeaseGrantAction>(std::move(params));
  call->waitForResponse();
  auto v3resp = call->ParseResponse();
  return std::make_shared<etcd::KeepAlive>(*this, ttl,
                                           v3resp.get_value().kvs.lease());
}

etcd::Response etcd::SyncClient::leaserevoke(int64_t lease_id) {
  return Response::create(this->leaserevoke_internal(lease_id));
}

std::shared_ptr<etcdv3::AsyncLeaseRevokeAction>
etcd::SyncClient::leaserevoke_internal(int64_t lease_id) {
  etcdv3::ActionParameters params;
  params.lease_id = lease_id;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  // leaserevoke: no timeout
  //
  // leaserevoke is special, as it calls `Finish()` inside the constructor, the
  // timeout may trigger a "SIGABRT" error on Mac
  //
  //  https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3/runs/6544444692?check_suite_focus=true
  //
  // params.grpc_timeout = this->grpc_timeout;
  params.lease_stub = stubs->leaseServiceStub.get();
  return std::make_shared<etcdv3::AsyncLeaseRevokeAction>(std::move(params));
}

etcd::Response etcd::SyncClient::leasetimetolive(int64_t lease_id) {
  return Response::create(this->leasetimetolive_internal(lease_id));
}

std::shared_ptr<etcdv3::AsyncLeaseTimeToLiveAction>
etcd::SyncClient::leasetimetolive_internal(int64_t lease_id) {
  etcdv3::ActionParameters params;
  params.lease_id = lease_id;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.lease_stub = stubs->leaseServiceStub.get();
  return std::make_shared<etcdv3::AsyncLeaseTimeToLiveAction>(
      std::move(params));
}

etcd::Response etcd::SyncClient::leases() {
  return Response::create(this->leases_internal());
}

std::shared_ptr<etcdv3::AsyncLeaseLeasesAction>
etcd::SyncClient::leases_internal() {
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.lease_stub = stubs->leaseServiceStub.get();
  return std::make_shared<etcdv3::AsyncLeaseLeasesAction>(std::move(params));
}

etcd::Response etcd::SyncClient::add_member(std::string const& peer_urls,
                                            bool is_learner) {
  return Response::create(this->add_member_internal(peer_urls, is_learner));
}

std::shared_ptr<etcdv3::AsyncAddMemberAction>
etcd::SyncClient::add_member_internal(std::string const& peer_urls,
                                      bool is_learner) {
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.cluster_stub = stubs->clusterServiceStub.get();

  std::vector<std::string> peer_urls_vector;
  std::istringstream iss(peer_urls);
  std::string peer_url;
  while (std::getline(iss, peer_url, ',')) {
    peer_urls_vector.push_back(peer_url);
  }

  params.is_learner = is_learner;
  params.peer_urls = peer_urls_vector;

  return std::make_shared<etcdv3::AsyncAddMemberAction>(std::move(params));
}

etcd::Response etcd::SyncClient::list_member() {
  return Response::create(this->list_member_internal());
}

std::shared_ptr<etcdv3::AsyncListMemberAction>
etcd::SyncClient::list_member_internal() {
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.cluster_stub = stubs->clusterServiceStub.get();
  return std::make_shared<etcdv3::AsyncListMemberAction>(std::move(params));
}

etcd::Response etcd::SyncClient::remove_member(const uint64_t member_id) {
  return Response::create(this->remove_member_internal(member_id));
}

std::shared_ptr<etcdv3::AsyncRemoveMemberAction>
etcd::SyncClient::remove_member_internal(const uint64_t member_id) {
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.cluster_stub = stubs->clusterServiceStub.get();
  params.member_id = member_id;

  return std::make_shared<etcdv3::AsyncRemoveMemberAction>(std::move(params));
}

etcd::Response etcd::SyncClient::lock(std::string const& key) {
  // routines in lock usually will be fast, less than 10 seconds.
  //
  // (base on our experiences in vineyard and GraphScope).
  static const int DEFAULT_LEASE_TTL_FOR_LOCK = 10;
  return this->lock(key, DEFAULT_LEASE_TTL_FOR_LOCK);
}

etcd::Response etcd::SyncClient::lock(std::string const& key, int lease_ttl) {
  auto keepalive = this->leasekeepalive(lease_ttl);
  return this->lock_internal(key, keepalive);
}

etcd::Response etcd::SyncClient::lock_internal(
    std::string const& key, std::shared_ptr<etcd::KeepAlive> const& keepalive) {
  etcdv3::ActionParameters params;
  params.key = key;
  params.lease_id = keepalive->Lease();
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.lock_stub = stubs->lockServiceStub.get();

  {
    std::lock_guard<std::mutex> lexical_scope_lock(mutex_for_keepalives);
    this->keep_alive_for_locks[keepalive->Lease()] = keepalive;
  }
  // synchronously wait the lock response to avoid deadlock
  auto call = std::make_shared<etcdv3::AsyncLockAction>(std::move(params));
  auto lock_resp = Response::create(std::move(call));
  // attach the lease id to the lock response
  lock_resp._value.leaseId = keepalive->Lease();
  {
    std::lock_guard<std::mutex> lexical_scope_lock(mutex_for_keepalives);
    if (lock_resp.is_ok()) {
      this->leases_for_locks[lock_resp.lock_key()] = keepalive->Lease();
    } else {
      this->keep_alive_for_locks.erase(keepalive->Lease());
    }
  }
  return lock_resp;
}

etcd::Response etcd::SyncClient::lock_with_lease(std::string const& key,
                                                 int64_t lease_id) {
  return Response::create(this->lock_with_lease_internal(key, lease_id));
}

std::shared_ptr<etcdv3::AsyncLockAction>
etcd::SyncClient::lock_with_lease_internal(std::string const& key,
                                           int64_t lease_id) {
  etcdv3::ActionParameters params;
  params.key = key;
  params.lease_id = lease_id;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.lock_stub = stubs->lockServiceStub.get();
  return std::make_shared<etcdv3::AsyncLockAction>(std::move(params));
}

etcd::Response etcd::SyncClient::unlock(std::string const& lock_key) {
  return Response::create(this->unlock_internal(lock_key));
}

std::shared_ptr<etcdv3::AsyncUnlockAction> etcd::SyncClient::unlock_internal(
    std::string const& lock_key) {
  etcdv3::ActionParameters params;
  params.key = lock_key;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.lock_stub = stubs->lockServiceStub.get();

  // issue a "unlock" request
  auto call = std::make_shared<etcdv3::AsyncUnlockAction>(std::move(params));

  // cancel the KeepAlive first, if it exists
  {
    std::lock_guard<std::mutex> lexical_scope_lock(mutex_for_keepalives);
    auto p_leases = this->leases_for_locks.find(lock_key);
    int64_t lock_lease_id = 0;
    if (p_leases != this->leases_for_locks.end()) {
      auto p_keeps_alive = this->keep_alive_for_locks.find(p_leases->second);
      if (p_keeps_alive != this->keep_alive_for_locks.end()) {
        this->keep_alive_for_locks.erase(p_keeps_alive);
      } else {
#ifndef NDEBUG
        std::cerr << "[warn] keepalive for lease not found" << std::endl;
#endif
      }
      lock_lease_id = p_leases->second;
      this->leases_for_locks.erase(p_leases);
    } else {
#ifndef NDEBUG
      std::cerr << "[warn] lease for lock not found" << std::endl;
#endif
    }
    if (lock_lease_id != 0) {
      this->leaserevoke(lock_lease_id);
    }
  }

  // asynchronously wait.
  return call;
}

etcd::Response etcd::SyncClient::txn(etcdv3::Transaction const& txn) {
  return Response::create(this->txn_internal(txn));
}

std::shared_ptr<etcdv3::AsyncTxnAction> etcd::SyncClient::txn_internal(
    etcdv3::Transaction const& txn) {
  etcdv3::ActionParameters params;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.kv_stub = stubs->kvServiceStub.get();
  return std::make_shared<etcdv3::AsyncTxnAction>(std::move(params), txn);
}

etcd::Response etcd::SyncClient::campaign(std::string const& name,
                                          int64_t lease_id,
                                          std::string const& value) {
  return Response::create(this->campaign_internal(name, lease_id, value));
}

std::shared_ptr<etcdv3::AsyncCampaignAction>
etcd::SyncClient::campaign_internal(std::string const& name, int64_t lease_id,
                                    std::string const& value) {
  etcdv3::ActionParameters params;
  params.name = name;
  params.lease_id = lease_id;
  params.value = value;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.election_stub = stubs->electionServiceStub.get();
  return std::make_shared<etcdv3::AsyncCampaignAction>(std::move(params));
}

etcd::Response etcd::SyncClient::proclaim(std::string const& name,
                                          int64_t lease_id,
                                          std::string const& key,
                                          int64_t revision,
                                          std::string const& value) {
  return Response::create(
      this->proclaim_internal(name, lease_id, key, revision, value));
}

std::shared_ptr<etcdv3::AsyncProclaimAction>
etcd::SyncClient::proclaim_internal(std::string const& name, int64_t lease_id,
                                    std::string const& key, int64_t revision,
                                    std::string const& value) {
  etcdv3::ActionParameters params;
  params.name = name;
  params.lease_id = lease_id;
  params.key = key;
  params.revision = revision;
  params.value = value;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.election_stub = stubs->electionServiceStub.get();
  return std::make_shared<etcdv3::AsyncProclaimAction>(std::move(params));
}

etcd::Response etcd::SyncClient::leader(std::string const& name) {
  return Response::create(this->leader_internal(name));
}

std::shared_ptr<etcdv3::AsyncLeaderAction> etcd::SyncClient::leader_internal(
    std::string const& name) {
  etcdv3::ActionParameters params;
  params.name = name;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.election_stub = stubs->electionServiceStub.get();
  return std::make_shared<etcdv3::AsyncLeaderAction>(std::move(params));
}

std::unique_ptr<etcd::SyncClient::Observer> etcd::SyncClient::observe(
    std::string const& name) {
  etcdv3::ActionParameters params;
  params.name.assign(name);
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.election_stub = stubs->electionServiceStub.get();
  std::unique_ptr<etcd::SyncClient::Observer> observer(new Observer());
  observer->action =
      std::make_shared<etcdv3::AsyncObserveAction>(std::move(params));
  return observer;
}

etcd::Response etcd::SyncClient::resign(std::string const& name,
                                        int64_t lease_id,
                                        std::string const& key,
                                        int64_t revision) {
  return Response::create(this->resign_internal(name, lease_id, key, revision));
}

std::shared_ptr<etcdv3::AsyncResignAction> etcd::SyncClient::resign_internal(
    std::string const& name, int64_t lease_id, std::string const& key,
    int64_t revision) {
  etcdv3::ActionParameters params;
  params.name = name;
  params.lease_id = lease_id;
  params.key = key;
  params.revision = revision;
  params.auth_token.assign(this->token_authenticator->renew_if_expired());
  params.grpc_timeout = this->grpc_timeout;
  params.election_stub = stubs->electionServiceStub.get();
  return std::make_shared<etcdv3::AsyncResignAction>(std::move(params));
}

const std::string& etcd::SyncClient::current_auth_token() const {
  return this->token_authenticator->renew_if_expired();
}

std::shared_ptr<grpc::Channel> etcd::SyncClient::grpc_channel() const {
  return this->channel;
}

etcd::SyncClient::Observer::~Observer() {
  if (this->action != nullptr) {
    this->action->CancelObserve();
    this->action = nullptr;
  }
}

etcd::Response etcd::SyncClient::Observer::WaitOnce() {
  if (this->action != nullptr) {
    return Response::create(this->action);
  } else {
    return Response{};
  }
}
