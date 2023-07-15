#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <chrono>
#include <iostream>
#include <thread>

#include "etcd/Client.hpp"

static const std::string etcd_url =
    etcdv3::detail::resolve_etcd_endpoints("http://127.0.0.1:2379");

TEST_CASE("setup") {
  etcd::Client etcd(etcd_url);
  etcd.rmdir("/test", true).wait();
}

TEST_CASE("add a new key") {
  etcd::Client etcd(etcd_url);
  etcd.rmdir("/test", true).wait();
  etcd::Response resp = etcd.add("/test/key1", "42").get();
  REQUIRE(0 == resp.error_code());
  CHECK("create" == resp.action());
  etcd::Value const& val = resp.value();
  CHECK("42" == val.as_string());
  CHECK("/test/key1" == val.key());
  CHECK(!val.is_dir());
  CHECK(0 < val.created_index());
  CHECK(0 < val.modified_index());
  CHECK(1 == val.version());
  CHECK(0 < resp.index());
  CHECK(etcd::ERROR_KEY_ALREADY_EXISTS ==
        etcd.add("/test/key1", "43").get().error_code());  // Key already exists
  CHECK(etcd::ERROR_KEY_ALREADY_EXISTS ==
        etcd.add("/test/key1", "42").get().error_code());  // Key already exists
  CHECK("etcd-cpp-apiv3: key already exists" ==
        etcd.add("/test/key1", "42").get().error_message());
}

TEST_CASE("read a value from etcd") {
  etcd::Client etcd(etcd_url);
  etcd::Response resp = etcd.get("/test/key1").get();
  CHECK("get" == resp.action());
  REQUIRE(resp.is_ok());
  REQUIRE(0 == resp.error_code());
  CHECK("42" == resp.value().as_string());
  CHECK("" == etcd.get("/test").get().value().as_string());  // key points to a
                                                             // directory
}

TEST_CASE("simplified read") {
  etcd::Client etcd(etcd_url);
  CHECK("42" == etcd.get("/test/key1").get().value().as_string());
  CHECK(etcd::ERROR_KEY_NOT_FOUND ==
        etcd.get("/test/key2").get().error_code());  // Key not found
  CHECK("" ==
        etcd.get("/test/key2").get().value().as_string());  // Key not found
}

TEST_CASE("modify a key") {
  etcd::Client etcd(etcd_url);

  // get
  etcd::Response resp = etcd.get("/test/key1").get();
  REQUIRE(resp.is_ok());
  int64_t revision = resp.value().modified_index();
  CHECK("42" == resp.value().as_string());

  // modify
  resp = etcd.modify("/test/key1", "43").get();
  REQUIRE(0 == resp.error_code());  // overwrite
  CHECK("update" == resp.action());
  CHECK(etcd::ERROR_KEY_NOT_FOUND ==
        etcd.modify("/test/key2", "43").get().error_code());  // Key not found
  CHECK("43" == etcd.modify("/test/key1", "42").get().prev_value().as_string());

  // check previous
  resp = etcd.get("/test/key1", revision).get();
  REQUIRE(resp.is_ok());
  CHECK("42" == resp.value().as_string());
}

TEST_CASE("set a key") {
  etcd::Client etcd(etcd_url);
  etcd::Response resp = etcd.set("/test/key1", "43").get();
  REQUIRE(0 == resp.error_code());  // overwrite
  CHECK("set" == resp.action());
  CHECK(0 == etcd.set("/test/key2", "43").get().error_code());  // create new
  CHECK("43" == etcd.set("/test/key2", "44").get().prev_value().as_string());
  CHECK("" == etcd.set("/test/key3", "44").get().prev_value().as_string());
  CHECK(0 == etcd.set("/test", "42").get().error_code());  // Not a file

  // set with ttl
  resp = etcd.set("/test/key1", "50").get();
  REQUIRE(0 == resp.error_code());  // overwrite
  CHECK("set" == resp.action());
  CHECK("43" == resp.prev_value().as_string());
  resp = etcd.get("/test/key1").get();
  CHECK("50" == resp.value().as_string());
  CHECK(0 == resp.value().lease());
}

TEST_CASE("atomic compare-and-swap") {
  etcd::Client etcd(etcd_url);
  etcd.set("/test/key1", "42").wait();

  // modify success
  etcd::Response res = etcd.modify_if("/test/key1", "43", "42").get();
  REQUIRE(res.is_ok());
  CHECK("compareAndSwap" == res.action());
  CHECK("43" == res.value().as_string());

  // modify fails the second time
  res = etcd.modify_if("/test/key1", "44", "42").get();
  CHECK(!res.is_ok());
  CHECK(etcd::ERROR_COMPARE_FAILED == res.error_code());
  CHECK("etcd-cpp-apiv3: compare failed" == res.error_message());

  // modify fails on non-existing keys
  res = etcd.modify_if("/test/key222", "44", "42").get();
  CHECK(!res.is_ok());
  CHECK(etcd::ERROR_COMPARE_FAILED == res.error_code());
  CHECK("etcd-cpp-apiv3: compare failed" == res.error_message());
}

TEST_CASE("delete a value") {
  etcd::Client etcd(etcd_url);
  etcd::Response resp = etcd.rm("/test/key11111").get();
  CHECK(!resp.is_ok());
  CHECK(etcd::ERROR_KEY_NOT_FOUND == resp.error_code());
  CHECK("etcd-cpp-apiv3: key not found" == resp.error_message());

  int64_t index = etcd.get("/test/key1").get().index();
  int64_t create_index = etcd.get("/test/key1").get().value().created_index();
  int64_t modify_index = etcd.get("/test/key1").get().value().modified_index();
  int64_t version = etcd.get("/test/key1").get().value().version();

  int head_index = etcd.head().get().index();
  CHECK(index == head_index);

  resp = etcd.rm("/test/key1").get();
  CHECK("43" == resp.prev_value().as_string());
  CHECK("/test/key1" == resp.prev_value().key());
  CHECK(create_index == resp.prev_value().created_index());
  CHECK(modify_index == resp.prev_value().modified_index());
  CHECK(version == resp.prev_value().version());
  CHECK("delete" == resp.action());
  CHECK(create_index == resp.value().created_index());
  CHECK(modify_index == resp.value().modified_index());
  CHECK(version == resp.value().version());
  CHECK("43" == resp.value().as_string());
  CHECK("/test/key1" == resp.value().key());
}

TEST_CASE("atomic compare-and-delete based on prevValue") {
  etcd::Client etcd(etcd_url);
  etcd.set("/test/key1", "42").wait();

  etcd::Response res = etcd.rm_if("/test/key1", "43").get();
  CHECK(!res.is_ok());
  CHECK(etcd::ERROR_COMPARE_FAILED == res.error_code());
  CHECK("etcd-cpp-apiv3: compare failed" == res.error_message());

  res = etcd.rm_if("/test/key1", "42").get();
  REQUIRE(res.is_ok());
  CHECK("compareAndDelete" == res.action());
  CHECK("42" == res.prev_value().as_string());
}

TEST_CASE("atomic compare-and-delete based on prevIndex") {
  etcd::Client etcd(etcd_url);
  int64_t index = etcd.set("/test/key1", "42").get().index();

  etcd::Response res = etcd.rm_if("/test/key1", index - 1).get();
  CHECK(!res.is_ok());
  CHECK(etcd::ERROR_COMPARE_FAILED == res.error_code());
  CHECK("etcd-cpp-apiv3: compare failed" == res.error_message());

  res = etcd.rm_if("/test/key1", index).get();
  REQUIRE(res.is_ok());
  CHECK("compareAndDelete" == res.action());
  CHECK("42" == res.prev_value().as_string());
}

TEST_CASE("deep atomic compare-and-swap") {
  etcd::Client etcd(etcd_url);
  etcd.set("/test/key1", "42").wait();

  // modify success
  etcd::Response res = etcd.modify_if("/test/key1", "43", "42").get();
  int64_t index = res.index();
  REQUIRE(res.is_ok());
  CHECK("compareAndSwap" == res.action());
  CHECK("43" == res.value().as_string());

  // modify fails the second time
  res = etcd.modify_if("/test/key1", "44", "42").get();
  CHECK(!res.is_ok());
  CHECK(etcd::ERROR_COMPARE_FAILED == res.error_code());
  CHECK("etcd-cpp-apiv3: compare failed" == res.error_message());

  // succes with the correct index
  res = etcd.modify_if("/test/key1", "44", index).get();
  REQUIRE(res.is_ok());
  CHECK("compareAndSwap" == res.action());
  CHECK("44" == res.value().as_string());

  // index changes so second modify fails
  res = etcd.modify_if("/test/key1", "45", index).get();
  CHECK(!res.is_ok());
  CHECK(etcd::ERROR_COMPARE_FAILED == res.error_code());
  CHECK("etcd-cpp-apiv3: compare failed" == res.error_message());
}

TEST_CASE("using binary keys and values, raw char pointer doesn't work") {
  etcd::Client etcd(etcd_url);
  etcd.rmdir("/test", true).wait();
  {
    etcd::Response resp = etcd.put("/test/key1\0xyz", "42\0foo").get();
    REQUIRE(resp.is_ok());
  }
  {
    // should not exist
    etcd::Response resp = etcd.get(std::string("/test/key1\0xyz", 14)).get();
    CHECK(!resp.is_ok());
    CHECK(etcd::ERROR_KEY_NOT_FOUND == resp.error_code());
  }
  {
    // should exist
    etcd::Response resp = etcd.get(std::string("/test/key1", 10)).get();
    REQUIRE(resp.is_ok());
    CHECK(std::string("42") == resp.value().as_string());
    CHECK(std::string("42\0foo", 6) != resp.value().as_string());
  }
  {
    // should exist, but different value
    etcd::Response resp = etcd.get("/test/key1\0xyz").get();
    REQUIRE(resp.is_ok());
    CHECK(std::string("42") == resp.value().as_string());
    CHECK(std::string("42\0foo", 6) != resp.value().as_string());
  }
}

TEST_CASE("using binary keys and values, std::string is ok for \\0") {
  etcd::Client etcd(etcd_url);
  etcd.rmdir("/test", true).wait();
  {
    etcd::Response resp =
        etcd.put(std::string("/test/key1\0xyz", 14), std::string("42\0foo", 6))
            .get();
    REQUIRE(resp.is_ok());
  }
  {
    // should not exist
    etcd::Response resp = etcd.get("/test/key1").get();
    CHECK(!resp.is_ok());
    CHECK(etcd::ERROR_KEY_NOT_FOUND == resp.error_code());
  }
  {
    // should exist
    etcd::Response resp = etcd.get(std::string("/test/key1\0xyz", 14)).get();
    REQUIRE(resp.is_ok());
    CHECK(std::string("42") != resp.value().as_string());
    CHECK(std::string("42\0foo", 6) == resp.value().as_string());
  }
}

TEST_CASE("list a directory") {
  etcd::Client etcd(etcd_url);
  CHECK(0 == etcd.ls("/test/new_dir").get().keys().size());

  etcd.set("/test/new_dir/key1", "value1").wait();
  etcd::Response resp = etcd.ls("/test/new_dir").get();
  CHECK("get" == resp.action());
  REQUIRE(1 == resp.keys().size());
  REQUIRE(1 == resp.values().size());
  CHECK("/test/new_dir/key1" == resp.key(0));
  CHECK("value1" == resp.value(0).as_string());
  CHECK(resp.values()[0].as_string() == "value1");

  etcd.set("/test/new_dir/key2", "value2").wait();
  etcd.set("/test/new_dir/sub_dir", "value3").wait();

  resp = etcd.ls("/test/new_dir").get();
  CHECK("get" == resp.action());
  REQUIRE(3 == resp.keys().size());
  CHECK("/test/new_dir/key1" == resp.key(0));
  CHECK("/test/new_dir/key2" == resp.key(1));
  CHECK("/test/new_dir/sub_dir" == resp.key(2));
  CHECK("value1" == resp.value(0).as_string());
  CHECK("value2" == resp.value(1).as_string());
  CHECK(resp.values()[2].is_dir() == 0);

  CHECK(0 == etcd.ls("/test/new_dir/key1").get().error_code());

  // list with empty prefix
  resp = etcd.ls("").get();
  CHECK(0 == resp.error_code());
  CHECK(0 < resp.keys().size());

  CHECK(etcd.rmdir("/test/new_dir", true).get().is_ok());
}

TEST_CASE("list by range") {
  etcd::Client etcd(etcd_url);
  CHECK(0 == etcd.ls("/test/new_dir").get().keys().size());

  etcd.set("/test/new_dir/key0", "value0").wait();
  etcd.set("/test/new_dir/key1", "value1").wait();
  etcd.set("/test/new_dir/key2", "value2").wait();
  etcd.set("/test/new_dir/key3", "value3").wait();
  etcd.set("/test/new_dir/key4", "value4").wait();

  etcd::Response resp1 =
      etcd.ls("/test/new_dir/key1", "/test/new_dir/key3").get();
  REQUIRE(resp1.is_ok());
  CHECK("get" == resp1.action());
  REQUIRE(2 == resp1.keys().size());
  REQUIRE(2 == resp1.values().size());

  etcd::Response resp2 =
      etcd.ls("/test/new_dir/key1", "/test/new_dir/key4").get();
  REQUIRE(resp2.is_ok());
  CHECK("get" == resp2.action());
  REQUIRE(3 == resp2.keys().size());
  REQUIRE(3 == resp2.values().size());

  etcd::Response resp3 = etcd.ls("/test/new_dir/key1", {"\xC0\x80"}).get();
  REQUIRE(resp3.is_ok());
  CHECK("get" == resp3.action());
  REQUIRE(4 == resp3.keys().size());
  REQUIRE(4 == resp3.values().size());

  etcd::Response resp4 =
      etcd.ls("/test/new_dir/key1",
              etcdv3::detail::string_plus_one("/test/new_dir/key"))
          .get();
  REQUIRE(resp4.is_ok());
  CHECK("get" == resp4.action());
  REQUIRE(4 == resp4.keys().size());
  REQUIRE(4 == resp4.values().size());

  CHECK(0 == etcd.ls("/test/new_dir/key1").get().error_code());

  CHECK(etcd.rmdir("/test/new_dir", true).get().is_ok());
}

TEST_CASE("list by range, w/o values") {
  etcd::Client etcd(etcd_url);
  CHECK(0 == etcd.ls("/test/new_dir").get().keys().size());

  etcd.set("/test/new_dir/key0", "value0").wait();
  etcd.set("/test/new_dir/key1", "value1").wait();
  etcd.set("/test/new_dir/key2", "value2").wait();
  etcd.set("/test/new_dir/key3", "value3").wait();
  etcd.set("/test/new_dir/key4", "value4").wait();

  etcd::Response resp1 =
      etcd.ls("/test/new_dir/key1", "/test/new_dir/key2").get();
  REQUIRE(resp1.is_ok());
  CHECK("get" == resp1.action());
  REQUIRE(1 == resp1.keys().size());
  REQUIRE(1 == resp1.values().size());
  REQUIRE(resp1.values()[0].as_string() == "value1");

  etcd::Response resp2 =
      etcd.keys("/test/new_dir/key1", "/test/new_dir/key2").get();
  REQUIRE(resp1.is_ok());
  CHECK("get" == resp2.action());
  REQUIRE(1 == resp2.keys().size());
  REQUIRE(1 == resp2.values().size());
  REQUIRE(resp2.values()[0].as_string() == "");

  CHECK(etcd.rmdir("/test/new_dir", true).get().is_ok());
}

TEST_CASE("delete a directory") {
  etcd::Client etcd(etcd_url);

  etcd.set("/test/new_dir/key1", "value1").wait();
  etcd.set("/test/new_dir/key2", "value2").wait();
  etcd.set("/test/new_dir/key3", "value3").wait();

  CHECK(etcd::ERROR_KEY_NOT_FOUND ==
        etcd.rmdir("/test/new_dir").get().error_code());  // key not found
  etcd::Response resp = etcd.ls("/test/new_dir").get();

  resp = etcd.rmdir("/test/new_dir", true).get();
  CHECK("delete" == resp.action());
  REQUIRE(3 == resp.keys().size());
  CHECK("/test/new_dir/key1" == resp.key(0));
  CHECK("/test/new_dir/key2" == resp.key(1));
  CHECK("value1" == resp.value(0).as_string());
  CHECK("value2" == resp.value(1).as_string());

  resp = etcd.rmdir("/test/dirnotfound", true).get();
  CHECK(!resp.is_ok());
  CHECK(etcd::ERROR_KEY_NOT_FOUND == resp.error_code());
  CHECK("etcd-cpp-apiv3: key not found" == resp.error_message());

  resp = etcd.rmdir("/test/new_dir", false).get();
  CHECK(!resp.is_ok());
  CHECK(etcd::ERROR_KEY_NOT_FOUND == resp.error_code());
  CHECK("etcd-cpp-apiv3: key not found" == resp.error_message());
}

TEST_CASE("delete all keys with rmdir(\"\", true)") {
  etcd::Client etcd(etcd_url);
  etcd.rmdir("", true).wait();

  etcd.set("/test/new_dir/key1", "value1").wait();
  etcd.set("/test/new_dir/key2", "value2").wait();
  etcd.set("/test/new_dir/key3", "value3").wait();

  auto resp = etcd.rmdir("", true).get();
  CHECK(resp.is_ok());
  CHECK(resp.values().size() == 3);
}

TEST_CASE("delete by range") {
  etcd::Client etcd(etcd_url);

  CHECK(etcd::ERROR_KEY_NOT_FOUND ==
        etcd.rmdir("/test/new_dir").get().error_code());  // key not found
  etcd::Response resp = etcd.ls("/test/new_dir").get();

  etcd.set("/test/new_dir/key1", "value1").wait();
  etcd.set("/test/new_dir/key2", "value2").wait();
  etcd.set("/test/new_dir/key3", "value3").wait();
  etcd.set("/test/new_dir/key4", "value4").wait();

  resp = etcd.rmdir("/test/new_dir/key1", "/test/new_dir/key3").get();
  CHECK("delete" == resp.action());
  REQUIRE(2 == resp.keys().size());
  CHECK("/test/new_dir/key1" == resp.key(0));
  CHECK("/test/new_dir/key2" == resp.key(1));
  CHECK("value1" == resp.value(0).as_string());
  CHECK("value2" == resp.value(1).as_string());
}

TEST_CASE("wait for a value change") {
  etcd::Client etcd(etcd_url);
  etcd.set("/test/key1", "42").wait();

  pplx::task<etcd::Response> res = etcd.watch("/test/key1");
  CHECK(!res.is_done());
  std::this_thread::sleep_for(std::chrono::seconds(1));
  CHECK(!res.is_done());

  etcd.set("/test/key1", "43").get();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  REQUIRE(res.is_done());
  REQUIRE("set" == res.get().action());
  CHECK("43" == res.get().value().as_string());
  CHECK("42" == res.get().prev_value().as_string());
}

TEST_CASE("wait for a directory change") {
  etcd::Client etcd(etcd_url);

  pplx::task<etcd::Response> res = etcd.watch("/test", true);
  CHECK(!res.is_done());
  std::this_thread::sleep_for(std::chrono::seconds(1));
  CHECK(!res.is_done());

  etcd.add("/test/key4", "44").wait();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  REQUIRE(res.is_done());
  CHECK("create" == res.get().action());
  CHECK("44" == res.get().value().as_string());

  pplx::task<etcd::Response> res2 = etcd.watch("/test", true);
  CHECK(!res2.is_done());
  std::this_thread::sleep_for(std::chrono::seconds(1));
  CHECK(!res2.is_done());

  etcd.set("/test/key4", "45").wait();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  REQUIRE(res2.is_done());
  CHECK("set" == res2.get().action());
  CHECK("45" == res2.get().value().as_string());
}

TEST_CASE("watch changes in the past") {
  etcd::Client etcd(etcd_url);
  REQUIRE(0 == etcd.rmdir("/test", true).get().error_code());
  int64_t index = etcd.set("/test/key1", "42").get().index();

  etcd.set("/test/key1", "43").wait();
  etcd.set("/test/key1", "44").wait();
  etcd.set("/test/key1", "45").wait();

  int64_t head_index = etcd.head().get().index();
  CHECK(index + 3 == head_index);

  etcd::Response res = etcd.watch("/test/key1", ++index).get();
  CHECK("set" == res.action());
  CHECK("43" == res.value().as_string());
  CHECK("42" == res.prev_value().as_string());

  res = etcd.watch("/test/key1", ++index).get();
  CHECK("set" == res.action());
  CHECK("44" == res.value().as_string());

  res = etcd.watch("/test", ++index, true).get();
  CHECK("set" == res.action());
  CHECK("45" == res.value().as_string());
}

TEST_CASE("watch range changes in the past") {
  etcd::Client etcd(etcd_url);
  REQUIRE(0 == etcd.rmdir("/test", true).get().error_code());
  int64_t index = etcd.set("/test/key1", "42").get().index();

  etcd.set("/test/key1", "43").wait();
  etcd.set("/test/key2", "44").wait();
  etcd.set("/test/key3", "45").wait();
  etcd.set("/test/key4", "45").wait();

  int64_t head_index = etcd.head().get().index();
  CHECK(index + 4 == head_index);

  etcd::Response res;

  res = etcd.watch("/test/key1", "/test/key4", index).get();
  CHECK(4 == res.events().size());
  res = etcd.watch("/test/key1", "/test/key5", index).get();
  CHECK(5 == res.events().size());
  res = etcd.watch("/test/key1", "/test/key4", ++index).get();
  CHECK(3 == res.events().size());
  res = etcd.watch("/test/key1", "/test/key5", ++index).get();
  CHECK(3 == res.events().size());
}

TEST_CASE("watch multiple keys and use promise") {
  etcd::Client etcd(etcd_url);

  int64_t start_index = etcd.set("/test/key1", "value1").get().index();
  etcd.set("/test/key2", "value2").get();

  pplx::task<size_t> res =
      etcd.watch("/test", start_index, true)
          .then([](pplx::task<etcd::Response> const& resp_task) -> size_t {
            auto const& resp = resp_task.get();
            return resp.events().size();
          });
  size_t event_size = res.get();
  CHECK(2 == event_size);
}

TEST_CASE("lease grant") {
  etcd::Client etcd(etcd_url);
  etcd::Response res = etcd.leasegrant(60).get();
  REQUIRE(res.is_ok());
  CHECK(60 == res.value().ttl());
  CHECK(0 < res.value().lease());
  int64_t leaseid = res.value().lease();

  res = etcd.set("/test/key1", "43", leaseid).get();
  REQUIRE(0 == res.error_code());  // overwrite
  CHECK("set" == res.action());
  res = etcd.get("/test/key1").get();
  REQUIRE(0 == res.error_code());  // overwrite
  CHECK(leaseid == res.value().lease());

  // change with lease id
  res = etcd.leasegrant(10).get();
  leaseid = res.value().lease();
  res = etcd.set("/test/key1", "43", leaseid).get();
  REQUIRE(0 == res.error_code());  // overwrite
  CHECK("set" == res.action());
  res = etcd.get("/test/key1").get();
  REQUIRE(0 == res.error_code());  // overwrite
  CHECK(leaseid == res.value().lease());

  // failure to attach lease id
  res = etcd.set("/test/key1", "43", leaseid + 1).get();
  REQUIRE(!res.is_ok());
  REQUIRE(5 == res.error_code());
  CHECK("etcdserver: requested lease not found" == res.error_message());

  res = etcd.modify("/test/key1", "44", leaseid).get();
  REQUIRE(0 == res.error_code());  // overwrite
  CHECK("update" == res.action());
  CHECK(leaseid == res.value().lease());
  CHECK("44" == res.value().as_string());

  // failure to attach invalid lease id
  res = etcd.modify("/test/key1", "45", leaseid + 1).get();
  REQUIRE(!res.is_ok());
  REQUIRE(5 == res.error_code());
  CHECK("etcdserver: requested lease not found" == res.error_message());

  res = etcd.modify_if("/test/key1", "45", "44", leaseid).get();
  int64_t index = res.index();
  REQUIRE(res.is_ok());
  CHECK("compareAndSwap" == res.action());
  CHECK("45" == res.value().as_string());

  // failure to attach invalid lease id
  res = etcd.modify_if("/test/key1", "46", "45", leaseid + 1).get();
  REQUIRE(!res.is_ok());
  REQUIRE(5 == res.error_code());
  CHECK("etcdserver: requested lease not found" == res.error_message());

  // succes with the correct index & lease id
  res = etcd.modify_if("/test/key1", "44", index, leaseid).get();
  index = res.index();
  REQUIRE(res.is_ok());
  CHECK("compareAndSwap" == res.action());
  CHECK("44" == res.value().as_string());

  res = etcd.modify_if("/test/key1", "44", index, leaseid + 1).get();
  REQUIRE(!res.is_ok());
  REQUIRE(5 == res.error_code());
  CHECK("etcdserver: requested lease not found" == res.error_message());

  res = etcd.add("/test/key11111", "43", leaseid).get();
  REQUIRE(0 == res.error_code());
  CHECK("create" == res.action());
  CHECK(leaseid == res.value().lease());

  // failure to attach invalid lease id
  res = etcd.set("/test/key11111", "43", leaseid + 1).get();
  REQUIRE(!res.is_ok());
  REQUIRE(5 == res.error_code());
  CHECK("etcdserver: requested lease not found" == res.error_message());
}

TEST_CASE("lease list") {
  etcd::Client etcd(etcd_url);
  etcd::Response res = etcd.leasegrant(60).get();
  REQUIRE(res.is_ok());
  CHECK(60 == res.value().ttl());
  CHECK(0 < res.value().lease());
  int64_t leaseid = res.value().lease();

  etcd::Response leasesresp = etcd.leases().get();
  if (leasesresp.is_ok()) {
    REQUIRE(leasesresp.is_ok());
    auto const& leases = leasesresp.leases();
    REQUIRE(leases.size() > 0);
    CHECK(std::find(leases.begin(), leases.end(), leaseid) != leases.end());
  } else {
    REQUIRE(leasesresp.error_code() == etcdv3::ERROR_GRPC_UNIMPLEMENTED);
  }
}

TEST_CASE("cleanup") {
  etcd::Client etcd(etcd_url);
  REQUIRE(0 == etcd.rmdir("/test", true).get().error_code());
}
