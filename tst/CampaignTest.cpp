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

TEST_CASE("campaign and loadership using keepalive") {
  etcd::Client etcd(etcd_uri);
  auto keepalive = etcd.leasekeepalive(5).get();
  auto lease_id = keepalive->Lease();

  std::cout << lease_id << std::endl;
  std::string value = std::string("192.168.1.6:1880");
  auto resp1 = etcd.campaign("/leader", lease_id, value).get();
  if (0 == resp1.error_code()) {
    std::cout << "became leader: " << resp1.index() << std::endl;
  } else {
    std::cout << "error code: " << resp1.error_code()
              << "error message: " << resp1.error_message() << std::endl;
    assert(false);
  }

  std::cout << "finish campaign" << std::endl;

  auto resp2 = etcd.leader("/leader").get();
  std::cout << resp2.value().as_string() << std::endl;
  std::cout << resp2.value().key() << std::endl;

  std::cout << "finish leader" << std::endl;
}
