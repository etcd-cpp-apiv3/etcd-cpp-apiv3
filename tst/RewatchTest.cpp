#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <chrono>
#include <thread>

#include "etcd/Client.hpp"
#include "etcd/SyncClient.hpp"
#include "etcd/Watcher.hpp"

static const std::string etcd_url =
    etcdv3::detail::resolve_etcd_endpoints("http://127.0.0.1:2379");

static int watcher_called = 0;

void print_response(etcd::Response const& resp) {
  ++watcher_called;
  std::cout << "print response called" << std::endl;
  if (resp.error_code()) {
    std::cout << resp.error_code() << ": " << resp.error_message() << std::endl;
  } else {
    std::cout << resp.action() << " " << resp.value().as_string() << std::endl;
    std::cout << "Previous value: " << resp.prev_value().as_string()
              << std::endl;

    std::cout << "Events size: " << resp.events().size() << std::endl;
    for (auto const& ev : resp.events()) {
      std::cout << "Value change in events: "
                << static_cast<int>(ev.event_type())
                << ", prev kv = " << ev.prev_kv().key() << " -> "
                << ev.prev_kv().as_string() << ", kv = " << ev.kv().key()
                << " -> " << ev.kv().as_string() << std::endl;
    }
  }
}

void wait_for_connection(std::string endpoints) {
  // wait until the client connects to etcd server
  while (true) {
#ifndef _ETCD_NO_EXCEPTIONS
    try {
      etcd::Client client(endpoints);
      if (client.head().get().is_ok()) {
        break;
      }
    } catch (...) {
      // pass
    }
#else
    etcd::Client client(endpoints);
    if (client.head().get().is_ok()) {
      break;
    }
#endif
    sleep(1);
  }
}

void initialize_watcher(const std::string& endpoints, const std::string& prefix,
                        std::function<void(etcd::Response)> callback,
                        std::shared_ptr<etcd::Watcher>& watcher) {
  // wait until the endpoints turn to be available
  wait_for_connection(endpoints);
  etcd::Client client(endpoints);

  // Check if the failed one has been cancelled first
  if (watcher && watcher->Cancelled()) {
    std::cout << "watcher's reconnect loop been cancelled" << std::endl;
    return;
  }
  watcher.reset(new etcd::Watcher(client, prefix, callback, true));

  // Note that lambda requires `mutable`qualifier.
  watcher->Wait(
      [endpoints, prefix, callback,
       /* By reference for renewing */ &watcher](bool cancelled) mutable {
        if (cancelled) {
          std::cout << "watcher's reconnect loop stopped as been cancelled"
                    << std::endl;
          return;
        }
        initialize_watcher(endpoints, prefix, callback, watcher);
      });
}

TEST_CASE("watch should can be re-established") {
  const std::string my_prefix = "/test";

  // the watcher initialized in this way will auto re-connect to etcd
  std::shared_ptr<etcd::Watcher> watcher;
  initialize_watcher(etcd_url, my_prefix, print_response, watcher);

  // issue some changes to see if the watcher works
  for (int round = 0; round < 100000; ++round) {
#ifndef _ETCD_NO_EXCEPTIONS
    try {
      etcd::Client client(etcd_url);
      auto response =
          client.set(my_prefix + "/foo", "bar-" + std::to_string(round)).get();
    } catch (...) {
      // pass
    }
#else
    etcd::Client client(etcd_url);
    auto response =
        client.set(my_prefix + "/foo", "bar-" + std::to_string(round)).get();
#endif

    std::this_thread::sleep_for(std::chrono::seconds(2));
  }

  // cancel the worker
  watcher->Cancel();

  // the watcher has been cancelled and shouldn't work anymore
  for (int round = 10; round < 20; ++round) {
#ifndef _ETCD_NO_EXCEPTIONS
    try {
      etcd::Client client(etcd_url);
      auto response =
          client.set(my_prefix + "/foo", "bar-" + std::to_string(round)).get();
    } catch (...) {
      // pass
    }
#else
    etcd::Client client(etcd_url);
    auto response =
        client.set(my_prefix + "/foo", "bar-" + std::to_string(round)).get();
#endif

    std::this_thread::sleep_for(std::chrono::seconds(2));
  }
}
