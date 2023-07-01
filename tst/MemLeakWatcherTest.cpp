#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <future>
#include <memory>

#include "etcd/Client.hpp"
#include "etcd/KeepAlive.hpp"
#include "etcd/Watcher.hpp"

static const std::string etcd_url =
    etcdv3::detail::resolve_etcd_endpoints("http://127.0.0.1:2379");

static std::atomic_int watcher_called;

void print_response(etcd::Response const& resp) { watcher_called.fetch_add(1); }

/**
 * @brief emulate the behavior of creating watcher many times:
 *
 * 1. create a watcher
 * 2. change a value
 * 3. cancel the watcher
 */
void watch_once(etcd::Client& client, std::unique_ptr<etcd::Watcher>& watcher,
                const size_t round) {
  const std::string my_prefix = "/test";
  const std::string my_key = my_prefix + "/foo";
  watcher.reset(new etcd::Watcher(client, my_prefix, print_response, true));

  int k = watcher_called.load();
  client.set(my_key, "bar-" + std::to_string(round)).wait();
  while (true) {
    if (watcher_called.load() > k) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  // cancel the watcher
  watcher->Cancel();
}

TEST_CASE("watch shouldn't leak memory") {
  watcher_called.store(0);

  // issue some changes to see if the watcher works
  etcd::Client client(etcd_url);
  std::unique_ptr<etcd::Watcher> watcher;
  for (int round = 0;
       round < 10 /* update this value to make it run for longer */; ++round) {
    if (round % 50 == 0) {
      std::cout << "starting round " << round << std::endl;
    }
    watch_once(client, watcher, round);
  }

  std::cout << "watcher been called for " << watcher_called.load() << " times"
            << std::endl;
}
