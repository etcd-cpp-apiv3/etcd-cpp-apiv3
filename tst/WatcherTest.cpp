#include <catch.hpp>

#include "etcd/Watcher.hpp"
#include "etcd/SyncClient.hpp"

static std::string etcd_uri("http://127.0.0.1:2379");
static int watcher_called = 0;

void printResponse(etcd::Response const & resp)
{
  ++watcher_called;
  std::cout << "print response called" << std::endl;
  if (resp.error_code())
    std::cout << resp.error_code() << ": " << resp.error_message() << std::endl;
  else
  {
    std::cout << resp.action() << " " << resp.value().as_string() << std::endl;
    std::cout << "Previous value: " << resp.prev_value().as_string() << std::endl;
  }
}

TEST_CASE("create watcher with cancel")
{
  
  etcd::SyncClient etcd(etcd_uri);
  etcd.rmdir("/test", true);

  watcher_called = 0;
  etcd::Watcher watcher(etcd_uri, "/test", printResponse);
  sleep(1);
  etcd.set("/test/key", "42");
  etcd.set("/test/key", "43");
  etcd.rm("/test/key");
  etcd.set("/test/key", "44");
  sleep(1);
  CHECK(4 == watcher_called);
  watcher.Cancel();
  etcd.set("/test/key", "50");
  etcd.set("/test/key", "51");
  sleep(1);
  CHECK(4 == watcher_called);

  etcd.rmdir("/test", true);

}

TEST_CASE("create watcher")
{
  
  etcd::SyncClient etcd(etcd_uri);
  etcd.rmdir("/test", true);

  watcher_called = 0;
  {
    etcd::Watcher watcher(etcd_uri, "/test", printResponse);
    sleep(1);
    etcd.set("/test/key", "42");
    etcd.set("/test/key", "43");
  }
  
  CHECK(2 == watcher_called);
  etcd.rmdir("/test", true).error_code();
}

// TEST_CASE("request cancellation")
// {
//   etcd::Client etcd(etcd_uri);
//   etcd.set("/test/key1", "42").wait();

//   pplx::task<etcd::Response> res = etcd.watch("/test/key1");
//   CHECK(!res.is_done());

//   etcd.cancel_operations();

//   sleep(1);
//   REQUIRE(res.is_done());
//   try
//   {
//     res.wait();
//   }
//   catch(pplx::task_canceled const & ex)
//   {
//     std::cout << "pplx::task_canceled: " << ex.what() << "\n";
//   }
//   catch(std::exception const & ex)
//   {
//     std::cout << "std::exception: " << ex.what() << "\n";
//   }
// }

