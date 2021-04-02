#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "etcd/Client.hpp"
#include "etcd/KeepAlive.hpp"


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
    // using a duration longer than default lease TTL for lock (see: DEFAULT_LEASE_TTL_FOR_LOCK)
    std::this_thread::sleep_for(std::chrono::seconds(15));

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

TEST_CASE("lock using lease")
{
  etcd::Client etcd("http://127.0.0.1:2379");

  bool failed = false;

  std::function<void (std::exception_ptr)> handler = [&failed](std::exception_ptr eptr) {
    try {
        if (eptr) {
            std::rethrow_exception(eptr);
        }
    } catch(const std::exception& e) {
        std::cerr << "Caught exception \"" << e.what() << "\"\n";
        failed = true;
    }
  };

  // with handler
  {
    // grant lease and keep it alive
    int64_t lease_id = etcd.leasegrant(5).get().value().lease();
    etcd::KeepAlive keepalive(etcd, handler, 3, lease_id);

    REQUIRE(!failed);
    keepalive.Check();  // shouldn't throw

    // lock
    etcd::Response resp1 = etcd.lock("/test/abcd", lease_id).get();
    CHECK("lock" == resp1.action());
    REQUIRE(resp1.is_ok());
    REQUIRE(0 == resp1.error_code());

    REQUIRE(!failed);
    keepalive.Check();  // shouldn't throw

    std::this_thread::sleep_for(std::chrono::seconds(20));

    REQUIRE(!failed);
    keepalive.Check();  // shouldn't throw

    // unlock
    etcd::Response resp2 = etcd.unlock(resp1.lock_key()).get();
    CHECK("unlock" == resp2.action());
    REQUIRE(resp2.is_ok());
    REQUIRE(0 == resp2.error_code());

    REQUIRE(!failed);
    keepalive.Check();  // shouldn't throw
  }

  // without handler
  {
    // grant lease and keep it alive
    int64_t lease_id = etcd.leasegrant(5).get().value().lease();
    etcd::KeepAlive keepalive(etcd, 3, lease_id);

    REQUIRE(!failed);
    keepalive.Check();  // shouldn't throw

    // lock
    etcd::Response resp1 = etcd.lock("/test/abcd", lease_id).get();
    CHECK("lock" == resp1.action());
    REQUIRE(resp1.is_ok());
    REQUIRE(0 == resp1.error_code());

    REQUIRE(!failed);
    keepalive.Check();  // shouldn't throw

    std::this_thread::sleep_for(std::chrono::seconds(20));

    REQUIRE(!failed);
    keepalive.Check();  // shouldn't throw

    // unlock
    etcd::Response resp2 = etcd.unlock(resp1.lock_key()).get();
    CHECK("unlock" == resp2.action());
    REQUIRE(resp2.is_ok());
    REQUIRE(0 == resp2.error_code());

    REQUIRE(!failed);
    keepalive.Check();  // shouldn't throw
  }
}
