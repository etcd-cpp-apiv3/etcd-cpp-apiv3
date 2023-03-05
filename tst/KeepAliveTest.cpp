#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <chrono>
#include <thread>

#include "etcd/Client.hpp"
#include "etcd/KeepAlive.hpp"
#include "etcd/Response.hpp"
#include "etcd/SyncClient.hpp"
#include "etcd/Value.hpp"

static std::string etcd_uri = etcdv3::detail::resolve_etcd_endpoints(
    "http://127.0.0.1:2379,http://127.0.0.1:2479,http://127.0.0.1:2579");

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
  REQUIRE_THROWS(keepalive->Check());
}
