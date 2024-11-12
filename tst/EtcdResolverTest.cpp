#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <iostream>

#include "etcd/Client.hpp"

static const std::string etcd_v4_url =
    etcdv3::detail::resolve_etcd_endpoints("http://127.0.0.1:2379");
static const std::string etcd_v6_url =
    etcdv3::detail::resolve_etcd_endpoints("http://::1:2379");
// http://[ipv6]:port url
static const std::string etcd_ipv6_url =
    etcdv3::detail::resolve_etcd_endpoints("http://[::1]:2379");

TEST_CASE("test ipv4 connection") {
  std::cout << "ipv4 endpoints: " << etcd_v4_url << std::endl;
  etcd::Client etcd(etcd_v4_url);
  REQUIRE(etcd.head().get().is_ok());
}

TEST_CASE("test ipv6 connection") {
  std::cout << "ipv6 endpoints: " << etcd_v6_url << std::endl;
  etcd::Client etcd(etcd_v6_url);
  REQUIRE(etcd.head().get().is_ok());

  std::cout << "ipv6 endpoints: " << etcd_ipv6_url << std::endl;
  etcd::Client etcd1(etcd_ipv6_url);
  REQUIRE(etcd1.head().get().is_ok());
}
