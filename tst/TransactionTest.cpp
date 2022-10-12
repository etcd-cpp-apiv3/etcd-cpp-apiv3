#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <chrono>
#include <iostream>
#include <thread>

#include "etcd/Client.hpp"
#include "etcd/v3/Transaction.hpp"

static const std::string etcd_url("http://127.0.0.1:2379");

TEST_CASE("setup")
{
  etcd::Client etcd(etcd_url);
  etcd.rmdir("/test", true).wait();
}

TEST_CASE("add a new key")
{
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
    txn.reset_key("/test/x1");
    txn.init_compare("1", etcdv3::CompareResult::EQUAL, etcdv3::CompareTarget::VALUE);

    txn.reset_key("/test/x2");
    txn.init_compare("2", etcdv3::CompareResult::EQUAL, etcdv3::CompareTarget::VALUE);

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

TEST_CASE("cleanup")
{
  etcd::Client etcd(etcd_url);
  REQUIRE(0 == etcd.rmdir("/test", true).get().error_code());
}
