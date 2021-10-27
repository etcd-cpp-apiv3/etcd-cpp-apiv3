#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <iostream>

#include "etcd/Client.hpp"


TEST_CASE("setup with auth")
{
  etcd::Client *etcd = etcd::Client::WithUser("http://127.0.0.1:2379", "root", "root");
  etcd->rmdir("/test", true).wait();
}

TEST_CASE("add a new key after authenticate")
{
  etcd::Client *etcd = etcd::Client::WithUser("http://127.0.0.1:2379", "root", "root");
  etcd->rmdir("/test", true).wait();
  etcd::Response resp = etcd->add("/test/key1", "42").get();
  REQUIRE(0 == resp.error_code());
  CHECK("create" == resp.action());
  etcd::Value const & val = resp.value();
  CHECK("42" == val.as_string());
  CHECK("/test/key1" == val.key());
  CHECK(!val.is_dir());
  CHECK(0 < val.created_index());
  CHECK(0 < val.modified_index());
  CHECK(1 == val.version());
  CHECK(0 < resp.index());
  CHECK(etcd::ERROR_KEY_ALREADY_EXISTS == etcd->add("/test/key1", "43").get().error_code()); // Key already exists
  CHECK(etcd::ERROR_KEY_ALREADY_EXISTS == etcd->add("/test/key1", "42").get().error_code()); // Key already exists
  CHECK("Key already exists" == etcd->add("/test/key1", "42").get().error_message());
}

TEST_CASE("read a value from etcd")
{
  etcd::Client *etcd = etcd::Client::WithUser("http://127.0.0.1:2379", "root", "root");
  etcd::Response resp = etcd->get("/test/key1").get();
  CHECK("get" == resp.action());
  REQUIRE(resp.is_ok());
  REQUIRE(0 == resp.error_code());
  CHECK("42" == resp.value().as_string());
  CHECK("" == etcd->get("/test").get().value().as_string()); // key points to a directory
}

TEST_CASE("cleanup")
{
  etcd::Client *etcd = etcd::Client::WithUser("http://127.0.0.1:2379", "root", "root");
  REQUIRE(0 == etcd->rmdir("/test", true).get().error_code());
}
