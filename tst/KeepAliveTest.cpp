#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <chrono>
#include <thread>

#include "etcd/Client.hpp"
#include "etcd/KeepAlive.hpp"
#include "etcd/Response.hpp"
#include "etcd/SyncClient.hpp"
#include "etcd/Value.hpp"

static std::string etcd_uri =
    etcdv3::detail::resolve_etcd_endpoints("http://127.0.0.1:2379");

TEST_CASE("keepalive revoke and check if alive") {
  etcd::Client etcd(etcd_uri);

  // create a lease with 30 seconds TTL
  auto keepalive = etcd.leasekeepalive(30).get();
  auto lease_id = keepalive->Lease();

  // revoke the lease before it reaches its TTL
  etcd.leaserevoke(lease_id).wait();

  // retrieves its TTL again, and it is now -1
  auto response = etcd.leasetimetolive(lease_id).get();
  REQUIRE(response.value().ttl() == -1);

  // shorter than the TLL, or no sleep
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // expect keep_alive->Check() to throw exception
#ifndef _ETCD_NO_EXCEPTIONS
  REQUIRE_THROWS(keepalive->Check());
#endif
}

TEST_CASE("keepalive won't expire") {
  etcd::Client etcd(etcd_uri);

  const int64_t ttl = 3;
  const std::string key = "key";
  const std::string meta_str = "meta ....";

  etcd::Response resp = etcd.leasegrant(ttl).get();
  auto lease_id = resp.value().lease();
  etcd.add(key, meta_str, lease_id);

#ifndef _ETCD_NO_EXCEPTIONS
  std::function<void(std::exception_ptr)> handler =
      [](std::exception_ptr eptr) {
        try {
          if (eptr) {
            std::rethrow_exception(eptr);
          }
        } catch (const std::runtime_error& e) {
          std::cerr << "Connection failure \"" << e.what() << "\"\n";
        } catch (const std::out_of_range& e) {
          std::cerr << "Lease expiry \"" << e.what() << "\"\n";
        }
      };
#else
  std::function<void(std::exception_ptr)> handler;
#endif
  etcd::KeepAlive keepalive(etcd, handler, ttl, lease_id);
  std::this_thread::sleep_for(std::chrono::seconds(5));
}

TEST_CASE("keepalive auto-grant") {
  etcd::Client etcd(etcd_uri);

  // create a lease without pre-granted lease id
  auto keepalive = std::make_shared<etcd::KeepAlive>(etcd, 10 /* ttl */);
  auto lease_id = keepalive->Lease();
  REQUIRE(lease_id != 0);

  // sleep for a while, and cancel
  std::this_thread::sleep_for(std::chrono::seconds(5));
  keepalive->Cancel();
}
