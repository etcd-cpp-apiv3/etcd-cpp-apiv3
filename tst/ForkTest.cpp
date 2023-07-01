#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "etcd/Client.hpp"
#include "etcd/KeepAlive.hpp"

static const std::string etcd_url =
    etcdv3::detail::resolve_etcd_endpoints("http://127.0.0.1:2379");

TEST_CASE("fork: set in child and get from self") {
  pid_t pid = fork();
  REQUIRE(pid >= 0);

  if (pid == 0) {
    // child
    etcd::Client etcd(etcd_url);
    etcd.set("/test/fork-key1", "fork: abcdefgh").wait();
    std::cout << "child: set finished ..." << std::endl;
  } else {
    // self
    etcd::Client etcd(etcd_url);
    size_t check = 0;
    while (check < 10) {
      auto resp = etcd.get("/test/fork-key1").get();
      if (!resp.is_ok()) {
        check += 1;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        continue;
      } else {
        CHECK(resp.value().as_string() == "fork: abcdefgh");
        break;
      }
    }
    std::cout << "self: get finished ..." << std::endl;
  }
}
