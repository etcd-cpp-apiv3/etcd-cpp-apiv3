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

TEST_CASE("campaign and leadership using keepalive") {
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
  CHECK(0 == resp2.error_code());
  CHECK(value == resp2.value().as_string());
  CHECK(resp1.value().key() == resp2.value().key());
  std::cout << resp2.value().key() << std::endl;

  std::cout << "finish leader" << std::endl;

  auto resp3 = etcd.resign("/leader", resp1.value().lease(),
                           resp1.value().key(), resp1.value().created_index())
                   .get();
  CHECK(0 == resp3.error_code());

  std::cout << "finish resign" << std::endl;
}

TEST_CASE("concurrent campaign with grpc timeout") {
  std::string value1 = std::string("192.168.1.6:1880");
  std::string value2 = std::string("192.168.1.6:1890");

  auto fn1 = [&]() {
    etcd::Client etcd(etcd_uri);
    auto keepalive = etcd.leasekeepalive(5).get();
    auto lease_id = keepalive->Lease();
    auto resp1 = etcd.campaign("/leader", lease_id, value1).get();
    CHECK(0 == resp1.error_code());

    std::this_thread::sleep_for(std::chrono::seconds(10));

    // resign
    auto resp2 = etcd.resign("/leader", resp1.value().lease(),
                             resp1.value().key(), resp1.value().created_index())
                     .get();
    CHECK(0 == resp2.error_code());
  };

  auto fn2 = [&]() {
    etcd::Client etcd(etcd_uri);
    auto keepalive = etcd.leasekeepalive(5).get();
    auto lease_id = keepalive->Lease();

    // set client timeout
    etcd.set_grpc_timeout(std::chrono::seconds(3));

    std::this_thread::sleep_for(std::chrono::seconds(3));
    auto resp1 = etcd.campaign("/leader", lease_id, value2).get();
    std::cout << resp1.error_code() << resp1.error_message() << std::endl;
    CHECK(0 != resp1.error_code());

    // wait until success
    while (true) {
      resp1 = etcd.campaign("/leader", lease_id, value2).get();
      if (resp1.error_code() == 0) {
        // check value
        auto resp2 = etcd.leader("/leader").get();
        CHECK(0 == resp2.error_code());
        CHECK(value2 == resp2.value().as_string());
        CHECK(resp1.value().key() == resp2.value().key());

        // break the loop
        break;
      }
    }

    auto resp2 = etcd.resign("/leader", resp1.value().lease(),
                             resp1.value().key(), resp1.value().created_index())
                     .get();
    CHECK(0 == resp2.error_code());
  };

  std::thread t1(fn1);
  std::thread t2(fn2);

  t1.join();
  t2.join();
}
