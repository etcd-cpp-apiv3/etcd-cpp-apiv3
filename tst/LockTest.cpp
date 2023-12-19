#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "etcd/Client.hpp"
#include "etcd/KeepAlive.hpp"

static const std::string etcd_url =
    etcdv3::detail::resolve_etcd_endpoints("http://127.0.0.1:2379");

TEST_CASE("lock and unlock") {
  etcd::Client etcd(etcd_url);

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

TEST_CASE("double lock will fail") {
  etcd::Client etcd(etcd_url);

  // lock
  etcd::Response resp1 = etcd.lock("/test/abcd").get();
  CHECK("lock" == resp1.action());
  REQUIRE(resp1.is_ok());
  REQUIRE(0 == resp1.error_code());

  bool first_lock_release = false;
  std::string lock_key = resp1.lock_key();

  auto second_lock_thr = std::thread([&]() {
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
    // using a duration longer than default lease TTL for lock (see:
    // DEFAULT_LEASE_TTL_FOR_LOCK)
    std::this_thread::sleep_for(std::chrono::seconds(15));

    // unlock the first lock
    first_lock_release = true;

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

TEST_CASE("lock could be timeout") {
  etcd::Client etcd(etcd_url);

  // setup the timeout
  etcd.set_grpc_timeout(std::chrono::seconds(5));

  // lock
  etcd::Response resp1 = etcd.lock("/test/abcd").get();
  CHECK("lock" == resp1.action());
  REQUIRE(resp1.is_ok());
  REQUIRE(0 == resp1.error_code());

  auto lock_in_another_thread = std::thread([&]() {
    // lock again
    etcd::Response resp2 = etcd.lock("/test/abcd").get();
    CHECK("lock" == resp2.action());
    REQUIRE(resp2.is_grpc_timeout());
  });

  lock_in_another_thread.join();

  // cleanup: unlock the second lock
  etcd::Response resp5 = etcd.unlock(resp1.lock_key()).get();
  CHECK("unlock" == resp5.action());
  REQUIRE(resp5.is_ok());
  REQUIRE(0 == resp5.error_code());
}

TEST_CASE("lock using lease") {
  etcd::Client etcd(etcd_url);

  bool failed = false;

#ifndef _ETCD_NO_EXCEPTIONS
  std::function<void(std::exception_ptr)> handler =
      [&failed](std::exception_ptr eptr) {
        try {
          if (eptr) {
            std::rethrow_exception(eptr);
          }
        } catch (const std::exception& e) {
          std::cerr << "Caught exception \"" << e.what() << "\"\n";
          failed = true;
        }
      };
#else
  std::function<void(std::exception_ptr)> handler;
#endif

  // with handler
  {
    // grant lease and keep it alive
    int64_t lease_id = etcd.leasegrant(5).get().value().lease();
    etcd::KeepAlive keepalive(etcd, handler, 3, lease_id);

    REQUIRE(!failed);
    keepalive.Check();  // shouldn't throw

    // lock
    etcd::Response resp1 = etcd.lock_with_lease("/test/abcd", lease_id).get();
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
    etcd::Response resp1 = etcd.lock_with_lease("/test/abcd", lease_id).get();
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

TEST_CASE("concurrent lock & unlock") {
  etcd::Client etcd(etcd_url);
  std::string const lock_key = "/test/test_key";

  constexpr size_t trials = 192;

  std::function<void(std::string const&, const size_t)> locker =
      [&etcd](std::string const& key, const size_t index) {
        std::cout << "start lock for " << key << ", index is " << index
                  << std::endl;
        auto resp = etcd.lock(key).get();
        std::cout << "lock for " << index << " is ok, starts sleeping: ..."
                  << resp.error_message() << std::endl
                  << std::flush;
        REQUIRE(resp.is_ok());
        std::srand(index);
        size_t time_to_sleep = 1;
        std::this_thread::sleep_for(std::chrono::seconds(time_to_sleep));
        std::cout << "lock for " << index << " resumes from sleep: ..."
                  << resp.error_message() << std::endl
                  << std::flush;
        REQUIRE(etcd.unlock(resp.lock_key()).get().is_ok());
        std::cout << "thread " << index << " been unlocked" << std::endl
                  << std::flush;
      };

  std::vector<std::thread> locks(trials);
  for (size_t index = 0; index < trials; ++index) {
    locks[index] = std::thread(locker, lock_key, index);
  }

  for (size_t index = 0; index < trials; ++index) {
    locks[index].join();
  }
}

TEST_CASE("concurrent lock & unlock with a put in between") {
  etcd::Client etcd(etcd_url);
  std::string const lock_key = "/test/test_key";

  constexpr size_t trials = 128;

  std::function<void(std::string const&, const size_t)> locker =
      [&etcd](std::string const& key, const size_t index) {
        std::cout << "start lock for " << index << std::endl;
        auto resp = etcd.lock(key, true).get();
        std::cout << "lock for " << index << " is ok, start put and unlock: ..."
                  << resp.error_message() << std::endl;
        REQUIRE(resp.is_ok());
        auto put_resp =
            etcd.put("/test/test_put", "hello" + std::to_string(index)).get();
        REQUIRE(put_resp.is_ok());
        REQUIRE(etcd.unlock(resp.lock_key()).get().is_ok());
        std::cout << "thread " << index << " been unlocked" << std::endl;
      };

  std::vector<std::thread> locks(trials);
  for (size_t index = 0; index < trials; ++index) {
    locks[index] = std::thread(locker, lock_key, index);
  }

  for (size_t index = 0; index < trials; ++index) {
    locks[index].join();
  }
}
