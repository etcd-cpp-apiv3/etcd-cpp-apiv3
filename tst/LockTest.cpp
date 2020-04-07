#include <atomic>
#include <catch.hpp>
#include <iostream>
#include <thread>

#include "etcd/Client.hpp"


TEST_CASE("lock and unlock")
{
  etcd::Client etcd("http://127.0.0.1:2379");

  // lock
  etcd::Response resp1 = etcd.lock("/test/abcd").get();
  CHECK("lock" == resp1.action());
  REQUIRE(resp1.is_ok());
  REQUIRE(0 == resp1.error_code());

  // unlock
  etcd::Response resp2 = etcd.unlock(resp1.lock_key()).get();
  CHECK("unlock" == resp2.action());
  REQUIRE(resp2.is_ok());
  REQUIRE(0 == resp2.error_code());
}


TEST_CASE("double lock will fail")
{
  etcd::Client etcd("http://127.0.0.1:2379");

  // lock
  etcd::Response resp1 = etcd.lock("/test/abcd").get();
  CHECK("lock" == resp1.action());
  REQUIRE(resp1.is_ok());
  REQUIRE(0 == resp1.error_code());

  bool first_lock_release = false;
  std::string lock_key = resp1.lock_key();

  auto second_lock_thr = std::thread([&](){
    // lock again
    etcd::Response resp2 = etcd.lock("/test/abcd").get();
    CHECK("lock" == resp2.action());
    REQUIRE(resp2.is_ok());
    REQUIRE(0 == resp2.error_code());
    lock_key = resp2.lock_key();

    // will success after first lock released.
    REQUIRE(first_lock_release);
  });

  auto first_lock_thr = std::thread([&]() {
    // check the lock key exists.
    etcd::Response resp3 = etcd.get(resp1.lock_key()).get();
    CHECK("get" == resp3.action());
    REQUIRE(resp3.is_ok());
    REQUIRE(0 == resp3.error_code());

    // create a duration
    first_lock_release = true;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // unlock the first lock
    etcd::Response resp4 = etcd.unlock(lock_key).get();
    CHECK("unlock" == resp4.action());
    REQUIRE(resp4.is_ok());
    REQUIRE(0 == resp4.error_code());
  });

  first_lock_thr.join();
  second_lock_thr.join();

  // cleanup: unlock the second lock
  etcd::Response resp5 = etcd.unlock(lock_key).get();
  CHECK("unlock" == resp5.action());
  REQUIRE(resp5.is_ok());
  REQUIRE(0 == resp5.error_code());
}
