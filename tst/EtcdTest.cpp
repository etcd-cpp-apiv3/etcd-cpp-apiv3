#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <iostream>

#include "etcd/Client.hpp"


TEST_CASE("setup")
{
  etcd::Client etcd("http://127.0.0.1:2379");
  etcd.rmdir("/test", true).wait();
}


TEST_CASE("add a new key")
{
  
  etcd::Client etcd("http://127.0.0.1:2379");
  etcd.rmdir("/test", true).wait();
  etcd::Response resp = etcd.add("/test/key1", "42").get();
  REQUIRE(0 == resp.error_code());
  CHECK("create" == resp.action());
  etcd::Value const & val = resp.value();
  CHECK("42" == val.as_string());
  CHECK("/test/key1" == val.key());
  CHECK(!val.is_dir());
  CHECK(0 < val.created_index());
  CHECK(0 < val.modified_index());
  CHECK(0 < resp.index()); // X-Etcd-Index header value
  CHECK(105 == etcd.add("/test/key1", "43").get().error_code()); // Key already exists
  CHECK(105 == etcd.add("/test/key1", "42").get().error_code()); // Key already exists
  CHECK("Key already exists" == etcd.add("/test/key1", "42").get().error_message());
}


TEST_CASE("read a value from etcd")
{
  etcd::Client etcd("http://127.0.0.1:2379");
  etcd::Response resp = etcd.get("/test/key1").get();
  CHECK("get" == resp.action());
  REQUIRE(resp.is_ok());
  REQUIRE(0 == resp.error_code());
  CHECK("42" == resp.value().as_string());

  CHECK("" == etcd.get("/test").get().value().as_string()); // key points to a directory
}


TEST_CASE("simplified read")
{
  etcd::Client etcd("http://127.0.0.1:2379");
  CHECK("42" == etcd.get("/test/key1").get().value().as_string());
  CHECK(100  == etcd.get("/test/key2").get().error_code()); // Key not found
  CHECK(""  == etcd.get("/test/key2").get().value().as_string()); // Key not found
}



TEST_CASE("modify a key")
{
  etcd::Client etcd("http://127.0.0.1:2379");
  etcd::Response resp = etcd.modify("/test/key1", "43").get();
  REQUIRE(0 == resp.error_code()); // overwrite
  CHECK("update" == resp.action());
  CHECK(100 == etcd.modify("/test/key2", "43").get().error_code()); // Key not found
  CHECK("43" == etcd.modify("/test/key1", "42").get().prev_value().as_string());
}


TEST_CASE("set a key")
{
  etcd::Client etcd("http://127.0.0.1:2379");
  etcd::Response resp = etcd.set("/test/key1", "43").get();
  REQUIRE(0  == resp.error_code()); // overwrite
  CHECK("set" == resp.action());
  CHECK(0  == etcd.set("/test/key2", "43").get().error_code()); // create new
  CHECK("43" == etcd.set("/test/key2", "44").get().prev_value().as_string());
  CHECK(""   == etcd.set("/test/key3", "44").get().prev_value().as_string());
  CHECK(0  == etcd.set("/test",      "42").get().error_code()); // Not a file

  //set with ttl
  resp = etcd.set("/test/key1", "50", 10).get();
  REQUIRE(0  == resp.error_code()); // overwrite
  CHECK("set" == resp.action());
  CHECK("43" == resp.prev_value().as_string());
  CHECK("50" == resp.value().as_string());
  CHECK( 0 < resp.value().lease());
}

TEST_CASE("atomic compare-and-swap")
{
  etcd::Client etcd("http://127.0.0.1:2379");
  etcd.set("/test/key1", "42").wait();

  // modify success
  etcd::Response res = etcd.modify_if("/test/key1", "43", "42").get();
  int index = res.index();
  REQUIRE(res.is_ok());
  CHECK("compareAndSwap" == res.action());
  CHECK("43" == res.value().as_string());

  // modify fails the second time
  res = etcd.modify_if("/test/key1", "44", "42").get();
  CHECK(!res.is_ok());
  CHECK(101 == res.error_code());
  CHECK("Compare failed" == res.error_message());

  // modify fails the second time
  res = etcd.modify_if("/test/key222", "44", "42").get();
  CHECK(!res.is_ok());
  CHECK(100 == res.error_code());
  CHECK("Key not found" == res.error_message());

}


TEST_CASE("delete a value")
{
  etcd::Client etcd("http://127.0.0.1:2379");
  etcd::Response resp = etcd.rm("/test/key11111").get();
  CHECK(!resp.is_ok());
  CHECK(100 == resp.error_code());
  CHECK("Key not found" == resp.error_message());

  int index = etcd.get("/test/key1").get().index();
  int create_index = etcd.get("/test/key1").get().value().created_index();
  int modify_index = etcd.get("/test/key1").get().value().modified_index();
  resp = etcd.rm("/test/key1").get();
  CHECK("43" == resp.prev_value().as_string());
  CHECK( "/test/key1" == resp.prev_value().key());
  CHECK( create_index == resp.prev_value().created_index());
  CHECK( modify_index == resp.prev_value().modified_index());
  CHECK("delete" == resp.action());
  CHECK( modify_index == resp.value().modified_index());
  CHECK( create_index == resp.value().created_index());
  CHECK("" == resp.value().as_string());
  CHECK( "/test/key1" == resp.value().key());
}

TEST_CASE("atomic compare-and-delete based on prevValue")
{
  etcd::Client etcd("http://127.0.0.1:2379");
  etcd.set("/test/key1", "42").wait();

  etcd::Response res = etcd.rm_if("/test/key1", "43").get();
  CHECK(!res.is_ok());
  CHECK(101 == res.error_code());
  CHECK("Compare failed" == res.error_message());

  res = etcd.rm_if("/test/key1", "42").get();
  REQUIRE(res.is_ok());
  CHECK("compareAndDelete" == res.action());
  CHECK("42" == res.prev_value().as_string());
}

TEST_CASE("atomic compare-and-delete based on prevIndex")
{
  etcd::Client etcd("http://127.0.0.1:2379");
  int index = etcd.set("/test/key1", "42").get().index();

  etcd::Response res = etcd.rm_if("/test/key1", index - 1).get();
  CHECK(!res.is_ok());
  CHECK(101 == res.error_code());
  CHECK("Compare failed" == res.error_message());

  res = etcd.rm_if("/test/key1", index).get();
  REQUIRE(res.is_ok());
  CHECK("compareAndDelete" == res.action());
  CHECK("42" == res.prev_value().as_string());
}


TEST_CASE("deep atomic compare-and-swap")
{
  etcd::Client etcd("http://127.0.0.1:2379");
  etcd.set("/test/key1", "42").wait();

  // modify success
  etcd::Response res = etcd.modify_if("/test/key1", "43", "42").get();
  int index = res.index();
  REQUIRE(res.is_ok());
  CHECK("compareAndSwap" == res.action());
  CHECK("43" == res.value().as_string());

  // modify fails the second time
  res = etcd.modify_if("/test/key1", "44", "42").get();
  CHECK(!res.is_ok());
  CHECK(101 == res.error_code());
  CHECK("Compare failed" == res.error_message());


  // succes with the correct index
  res = etcd.modify_if("/test/key1", "44", index).get();
  REQUIRE(res.is_ok());
  CHECK("compareAndSwap" == res.action());
  CHECK("44" == res.value().as_string());

  // index changes so second modify fails
  res = etcd.modify_if("/test/key1", "45", index).get();
  CHECK(!res.is_ok());
  CHECK(101 == res.error_code());
  CHECK("Compare failed" == res.error_message());

}


//skip this test case
/*
TEST_CASE("create a directory")
{
  etcd::Client etcd("http://127.0.0.1:4001");
  etcd::Response resp = etcd.mkdir("/test/new_dir").get();
  CHECK("set" == resp.action());
  CHECK(resp.value().is_dir());
}
*/

TEST_CASE("list a directory")
{
  etcd::Client etcd("http://127.0.0.1:2379");
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
}

TEST_CASE("delete a directory")
{
  etcd::Client etcd("http://127.0.0.1:2379");

  //CHECK(108 == etcd.rmdir("/test/new_dir").get().error_code()); // Directory not empty
  etcd::Response resp = etcd.ls("/test/new_dir").get();


  resp = etcd.rmdir("/test/new_dir", true).get();
  int index = resp.index();
  CHECK("delete" == resp.action());
  REQUIRE(3 == resp.keys().size());
  CHECK("/test/new_dir/key1" == resp.key(0));
  CHECK("/test/new_dir/key2" == resp.key(1));
  CHECK("value1" == resp.value(0).as_string());
  CHECK("value2" == resp.value(1).as_string());


  resp = etcd.rmdir("/test/dirnotfound", true).get();
  CHECK(!resp.is_ok());
  CHECK(100 == resp.error_code());
  CHECK("Key not found" == resp.error_message());

  resp = etcd.rmdir("/test/new_dir", false).get();
  CHECK(!resp.is_ok());
  CHECK(100 == resp.error_code());
  CHECK("Key not found" == resp.error_message());
}

TEST_CASE("wait for a value change")
{
  etcd::Client etcd("http://127.0.0.1:2379");
  etcd.set("/test/key1", "42").wait();

  pplx::task<etcd::Response> res = etcd.watch("/test/key1");
  CHECK(!res.is_done());
  sleep(1);
  CHECK(!res.is_done());
  
  etcd.set("/test/key1", "43").get();
  sleep(1);
  REQUIRE(res.is_done());
  REQUIRE("set" == res.get().action());
  CHECK("43" == res.get().value().as_string());
  CHECK("42" == res.get().prev_value().as_string());
}

TEST_CASE("wait for a directory change")
{
  etcd::Client etcd("http://127.0.0.1:2379");

  pplx::task<etcd::Response> res = etcd.watch("/test", true);
  CHECK(!res.is_done());
  sleep(1);
  CHECK(!res.is_done());

  etcd.add("/test/key4", "44").wait();
  sleep(1);
  REQUIRE(res.is_done());
  CHECK("create" == res.get().action());
  CHECK("44" == res.get().value().as_string());

  pplx::task<etcd::Response> res2 = etcd.watch("/test", true);
  CHECK(!res2.is_done());
  sleep(1);
  CHECK(!res2.is_done());

  etcd.set("/test/key4", "45").wait();
  sleep(1);
  REQUIRE(res2.is_done());
  CHECK("set" == res2.get().action());
  CHECK("45" == res2.get().value().as_string());
}

TEST_CASE("watch changes in the past")
{
  etcd::Client etcd("http://127.0.0.1:2379");
  REQUIRE(0 == etcd.rmdir("/test", true).get().error_code());
  int index = etcd.set("/test/key1", "42").get().index();

  etcd.set("/test/key1", "43").wait();
  etcd.set("/test/key1", "44").wait();
  etcd.set("/test/key1", "45").wait();

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

TEST_CASE("watch multiple keys and use promise") {
  etcd::Client etcd("http://127.0.0.1:2379");

  int start_index = etcd.add("/test/key1", "value1").get().index();
  etcd.add("/test/key2", "value2").get();

  pplx::task<size_t> res = etcd.watch("/test", start_index, true)
    .then([](pplx::task<etcd::Response> const &resp_task) -> size_t {
      auto const &resp = resp_task.get();
      return resp.events().size();
    });
  size_t event_size = res.get();
  CHECK(2 == event_size);
}

TEST_CASE("lease grant")
{
  etcd::Client etcd("http://127.0.0.1:2379");
  etcd::Response res = etcd.leasegrant(60).get();
  REQUIRE(res.is_ok());
  CHECK(60 == res.value().ttl());
  CHECK(0 <  res.value().lease());
  int64_t leaseid = res.value().lease();

  res = etcd.set("/test/key1", "43", leaseid).get();
  REQUIRE(0  == res.error_code()); // overwrite
  CHECK("set" == res.action());
  CHECK(leaseid ==  res.value().lease());

  //change leaseid
  res = etcd.leasegrant(10).get();
  leaseid = res.value().lease();
  res = etcd.set("/test/key1", "43", leaseid).get();
  REQUIRE(0  == res.error_code()); // overwrite
  CHECK("set" == res.action());
  CHECK(leaseid == res.value().lease());

  //failure to attach lease id
  res = etcd.set("/test/key1", "43", leaseid+1).get();
  REQUIRE(!res.is_ok());
  REQUIRE(5  == res.error_code()); 
  CHECK("etcdserver: requested lease not found" == res.error_message());

  res = etcd.modify("/test/key1", "44", leaseid).get();
  REQUIRE(0  == res.error_code()); // overwrite
  CHECK("update" == res.action());
  CHECK(leaseid ==  res.value().lease());
  CHECK("44" ==  res.value().as_string());

  //failure to attach lease id
  res = etcd.modify("/test/key1", "45", leaseid+1).get();
  REQUIRE(!res.is_ok());
  REQUIRE(5  == res.error_code()); 
  CHECK("etcdserver: requested lease not found" == res.error_message());

  res = etcd.modify_if("/test/key1", "45", "44", leaseid).get();
  int index = res.index();
  REQUIRE(res.is_ok());
  CHECK("compareAndSwap" == res.action());
  CHECK("45" == res.value().as_string());

  //failure to attach lease id
  res = etcd.modify_if("/test/key1", "46", "45", leaseid+1).get();
  REQUIRE(!res.is_ok());
  REQUIRE(5  == res.error_code()); 
  CHECK("etcdserver: requested lease not found" == res.error_message());

  // succes with the correct index
  res = etcd.modify_if("/test/key1", "44", index, leaseid).get();
  index = res.index();
  REQUIRE(res.is_ok());
  CHECK("compareAndSwap" == res.action());
  CHECK("44" == res.value().as_string());

  res = etcd.modify_if("/test/key1", "44", index, leaseid+1).get();
  REQUIRE(!res.is_ok());
  REQUIRE(5  == res.error_code()); 
  CHECK("etcdserver: requested lease not found" == res.error_message());

  res = etcd.add("/test/key11111", "43", leaseid).get();
  REQUIRE(0  == res.error_code()); 
  CHECK("create" == res.action());
  CHECK(leaseid ==  res.value().lease());

  //failure to attach lease id
  res = etcd.set("/test/key11111", "43", leaseid+1).get();
  REQUIRE(!res.is_ok());
  REQUIRE(5  == res.error_code()); 
  CHECK("etcdserver: requested lease not found" == res.error_message());
}

TEST_CASE("cleanup")
{
  etcd::Client etcd("http://127.0.0.1:2379");
  REQUIRE(0 == etcd.rmdir("/test", true).get().error_code());
}



