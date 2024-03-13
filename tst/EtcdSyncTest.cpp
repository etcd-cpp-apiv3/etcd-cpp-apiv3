#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <chrono>
#include <thread>

#include "etcd/SyncClient.hpp"

static const std::string etcd_url =
    etcdv3::detail::resolve_etcd_endpoints("http://127.0.0.1:2379");

TEST_CASE("sync operations") {
  etcd::SyncClient etcd(etcd_url);
  etcd.rmdir("/test", true);

  etcd::Response res;
  int64_t index;

  // add
  CHECK(0 == etcd.add("/test/key1", "42").error_code());
  CHECK(etcd::ERROR_KEY_ALREADY_EXISTS ==
        etcd.add("/test/key1", "41").error_code());  // Key already exists
  CHECK("42" == etcd.get("/test/key1").value().as_string());

  // modify
  CHECK(0 == etcd.modify("/test/key1", "43").error_code());
  CHECK(etcd::ERROR_KEY_NOT_FOUND ==
        etcd.modify("/test/key2", "43").error_code());  // Key not found
  res = etcd.modify("/test/key1", "42");
  CHECK(0 == res.error_code());
  CHECK("43" == res.prev_value().as_string());

  // set
  CHECK(0 == etcd.set("/test/key1", "43").error_code());  // overwrite
  CHECK(0 == etcd.set("/test/key2", "43").error_code());  // create new
  CHECK("43" == etcd.set("/test/key2", "44").prev_value().as_string());
  CHECK("" == etcd.set("/test/key3", "44").prev_value().as_string());

  // get
  CHECK("43" == etcd.get("/test/key1").value().as_string());
  CHECK("44" == etcd.get("/test/key2").value().as_string());
  CHECK("44" == etcd.get("/test/key3").value().as_string());
  CHECK(etcd::ERROR_KEY_NOT_FOUND ==
        etcd.get("/test/key4").error_code());  // key not found

  // rm
  CHECK(3 == etcd.ls("/test").keys().size());
  CHECK(0 == etcd.rm("/test/key1").error_code());
  CHECK(2 == etcd.ls("/test").keys().size());

  // ls
  CHECK(0 == etcd.ls("/test/new_dir").keys().size());
  etcd.set("/test/new_dir/key1", "value1");
  etcd.set("/test/new_dir/key2", "value2");
  CHECK(2 == etcd.ls("/test/new_dir").keys().size());

  // keys
  etcd.set("/test/new_dir/key1", "value1");
  etcd.set("/test/new_dir/key2", "value2");
  CHECK(2 == etcd.keys("/test/new_dir").keys().size());

  // rmdir
  CHECK(etcd::ERROR_KEY_NOT_FOUND ==
        etcd.rmdir("/test/new_dir").error_code());  // key not found
  CHECK(0 == etcd.rmdir("/test/new_dir", true).error_code());

  // compare and swap
  etcd.set("/test/key1", "42");
  index = etcd.modify_if("/test/key1", "43", "42").index();
  CHECK(etcd::ERROR_COMPARE_FAILED ==
        etcd.modify_if("/test/key1", "44", "42").error_code());
  REQUIRE(etcd.modify_if("/test/key1", "44", index).is_ok());
  CHECK(etcd::ERROR_COMPARE_FAILED ==
        etcd.modify_if("/test/key1", "45", index).error_code());

  // atomic compare-and-delete based on prevValue
  etcd.set("/test/key1", "42");
  CHECK(etcd::ERROR_COMPARE_FAILED ==
        etcd.rm_if("/test/key1", "43").error_code());
  res = etcd.rm_if("/test/key1", "42");
  CHECK(
      (0 == res.error_code() || etcd::ERROR_KEY_NOT_FOUND == res.error_code()));

  // atomic compare-and-delete based on prevIndex
  index = etcd.set("/test/key1", "42").index();
  CHECK(etcd::ERROR_COMPARE_FAILED ==
        etcd.rm_if("/test/key1", index - 1).error_code());
  res = etcd.rm_if("/test/key1", index);
  CHECK(
      (0 == res.error_code() || etcd::ERROR_KEY_NOT_FOUND == res.error_code()));

  // leasegrant
  res = etcd.leasegrant(60);
  REQUIRE(res.is_ok());
  CHECK(60 == res.value().ttl());
  CHECK(0 < res.value().lease());
  int64_t leaseid = res.value().lease();

  // add with lease
  res = etcd.add("/test/key1111", "43", leaseid);
  REQUIRE(0 == res.error_code());  // overwrite
  CHECK("create" == res.action());
  CHECK(leaseid == res.value().lease());

  // set with lease
  res = etcd.set("/test/key1", "43", leaseid);
  REQUIRE(0 == res.error_code());
  CHECK("set" == res.action());
  res = etcd.get("/test/key1");
  CHECK(leaseid == res.value().lease());

  // modify with lease
  res = etcd.modify("/test/key1", "44", leaseid);
  REQUIRE(0 == res.error_code());
  CHECK("update" == res.action());
  CHECK(leaseid == res.value().lease());
  CHECK("44" == res.value().as_string());

  res = etcd.modify_if("/test/key1", "45", "44", leaseid);
  index = res.index();
  REQUIRE(res.is_ok());
  CHECK("compareAndSwap" == res.action());
  CHECK(leaseid == res.value().lease());
  CHECK("45" == res.value().as_string());

  res = etcd.modify_if("/test/key1", "44", index, leaseid);
  index = res.index();
  REQUIRE(res.is_ok());
  CHECK("compareAndSwap" == res.action());
  CHECK(leaseid == res.value().lease());
  CHECK("44" == res.value().as_string());

  REQUIRE(0 == etcd.rmdir("/test", true).error_code());
}

TEST_CASE("wait for a value change") {
  etcd::SyncClient etcd(etcd_url);
  etcd.set("/test/key1", "42");

  std::thread watch_thrd([&]() {
    etcd::Response res = etcd.watch("/test/key1");
    REQUIRE("set" == res.action());
    CHECK("43" == res.value().as_string());
  });

  std::this_thread::sleep_for(std::chrono::seconds(1));
  etcd.set("/test/key1", "43");
  watch_thrd.join();

  REQUIRE(0 == etcd.rmdir("/test", true).error_code());
}

TEST_CASE("wait for a directory change") {
  etcd::SyncClient etcd(etcd_url);

  std::thread watch_thrd1([&]() {
    etcd::Response res = etcd.watch("/test", true);
    CHECK("create" == res.action());
    CHECK("44" == res.value().as_string());
  });

  std::this_thread::sleep_for(std::chrono::seconds(1));
  etcd.add("/test/key4", "44");
  watch_thrd1.join();

  std::thread watch_thrd2([&]() {
    etcd::Response res2 = etcd.watch("/test", true);
    CHECK("set" == res2.action());
    CHECK("45" == res2.value().as_string());
  });

  std::this_thread::sleep_for(std::chrono::seconds(1));
  etcd.set("/test/key4", "45");
  watch_thrd2.join();

  REQUIRE(0 == etcd.rmdir("/test", true).error_code());
}

TEST_CASE("watch changes in the past") {
  etcd::SyncClient etcd(etcd_url);

  int64_t index = etcd.set("/test/key1", "42").index();

  etcd.set("/test/key1", "43");
  etcd.set("/test/key1", "44");
  etcd.set("/test/key1", "45");

  etcd::Response res = etcd.watch("/test/key1", ++index);
  CHECK("set" == res.action());
  CHECK("43" == res.value().as_string());

  res = etcd.watch("/test/key1", ++index);
  CHECK("set" == res.action());
  CHECK("44" == res.value().as_string());

  res = etcd.watch("/test", ++index, true);
  CHECK("set" == res.action());
  CHECK("45" == res.value().as_string());

  REQUIRE(0 == etcd.rmdir("/test", true).error_code());
}

// TEST_CASE("request cancellation")
// {
//   etcd::Client etcd(etcd_url);
//   etcd.set("/test/key1", "42").wait();

//   pplx::task<etcd::Response> res = etcd.watch("/test/key1");
//   CHECK(!res.is_done());

//   etcd.cancel_operations();

//   std::this_thread::sleep_for(std::chrono::seconds(1));
//   REQUIRE(res.is_done());
//   try {
//     res.wait();
//   }
//   catch(pplx::task_canceled const & ex) {
//     std::cout << "pplx::task_canceled: " << ex.what() << "\n";
//   }
//   catch(std::exception const & ex) {
//     std::cout << "std::exception: " << ex.what() << "\n";
//   }
// }
