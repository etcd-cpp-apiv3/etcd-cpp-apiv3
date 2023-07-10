#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <chrono>
#include <iostream>
#include <thread>

#include "etcd/Client.hpp"
#include "etcd/v3/Transaction.hpp"

static const std::string etcd_url =
    etcdv3::detail::resolve_etcd_endpoints("http://127.0.0.1:2379");

TEST_CASE("setup") {
  etcd::Client etcd(etcd_url);
  etcd.rmdir("/test", true).wait();
}

TEST_CASE("add a new key") {
  etcd::Client etcd(etcd_url);
  etcd.rmdir("/test", true).wait();

  // do some put using txn
  {
    etcdv3::Transaction txn;
    txn.setup_put("/test/x1", "1");
    txn.setup_put("/test/x2", "2");
    txn.setup_put("/test/x3", "3");
    etcd::Response resp = etcd.txn(txn).get();
    REQUIRE(0 == resp.error_code());
  }

  // check the value
  {
    etcd::Response resp = etcd.get("/test/x1").get();
    REQUIRE(0 == resp.error_code());
    CHECK(resp.value().as_string() == "1");

    resp = etcd.get("/test/x2").get();
    REQUIRE(0 == resp.error_code());
    CHECK(resp.value().as_string() == "2");

    resp = etcd.get("/test/x3").get();
    REQUIRE(0 == resp.error_code());
    CHECK(resp.value().as_string() == "3");
  }

  // do some put and delete using txn
  {
    etcdv3::Transaction txn;

    // setup the conditions
    txn.add_compare_value("/test/x1", "1");
    txn.add_compare_value("/test/x2", "2");

    txn.setup_put("/test/x1", "111");
    txn.setup_delete("/test/x2");
    txn.setup_delete("/test/x3");
    txn.setup_put("/test/x4", "4");
    etcd::Response resp = etcd.txn(txn).get();
    REQUIRE(0 == resp.error_code());
  }

  // check the value
  {
    etcd::Response resp = etcd.get("/test/x1").get();
    REQUIRE(0 == resp.error_code());
    CHECK(resp.value().as_string() == "111");

    resp = etcd.get("/test/x2").get();
    REQUIRE(etcdv3::ERROR_KEY_NOT_FOUND == resp.error_code());

    resp = etcd.get("/test/x3").get();
    REQUIRE(etcdv3::ERROR_KEY_NOT_FOUND == resp.error_code());

    resp = etcd.get("/test/x4").get();
    REQUIRE(0 == resp.error_code());
    CHECK(resp.value().as_string() == "4");
  }
}

TEST_CASE("fetch & add") {
  etcd::Client etcd(etcd_url);
  etcd.rmdir("/test", true).wait();

  etcd.set("/test/key", "0").wait();

  auto fetch_and_add = [](etcd::Client& client,
                          std::string const& key) -> void {
    auto value = stoi(client.get(key).get().value().as_string());
    while (true) {
      auto txn = etcdv3::Transaction();
      txn.setup_compare_and_swap(key, std::to_string(value),
                                 std::to_string(value + 1));
      etcd::Response resp = client.txn(txn).get();
      if (resp.is_ok()) {
        break;
      }
      value = stoi(resp.value().as_string());
    }
  };

  // run 1000 times
  const size_t rounds = 100;
  std::atomic_size_t counter(0);

  std::vector<std::thread> threads;
  for (size_t i = 0; i < 10; ++i) {
    threads.emplace_back([&]() {
      while (counter.fetch_add(1) < rounds) {
        fetch_and_add(etcd, "/test/key");
      }
    });
  }
  for (auto& thr : threads) {
    thr.join();
  }

  // check the value
  {
    etcd::Response resp = etcd.get("/test/key").get();
    REQUIRE(0 == resp.error_code());
    CHECK(resp.value().as_string() == std::to_string(rounds));
  }
}

TEST_CASE("cleanup") {
  etcd::Client etcd(etcd_url);
  REQUIRE(0 == etcd.rmdir("/test", true).get().error_code());
}
