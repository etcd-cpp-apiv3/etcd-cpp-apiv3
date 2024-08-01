#include <cmath>
#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

#include "etcd/Client.hpp"

static const std::string etcd_url =
    etcdv3::detail::resolve_etcd_endpoints("http://127.0.0.1:2379");

pid_t new_etcd_pid = 0;

pid_t start_etcd_server(const std::string& etcd_path,
                        const std::vector<std::string>& args) {
  pid_t pid = fork();
  if (pid == -1) {
    std::cout << "Failed to fork process" << std::endl;
    exit(EXIT_FAILURE);
  } else if (pid == 0) {
    std::vector<char*> c_args;
    c_args.push_back(const_cast<char*>(etcd_path.c_str()));
    for (const auto& arg : args) {
      c_args.push_back(const_cast<char*>(arg.c_str()));
    }
    c_args.push_back(nullptr);

    if (execvp(etcd_path.c_str(), c_args.data()) == -1) {
      std::cout << "Failed to exec etcd process: " << std::strerror(errno)
                << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  return pid;
}

TEST_CASE("add member") {
  etcd::Client etcd(etcd_url);
  etcd::Response res = etcd.list_member().get();
  REQUIRE(res.is_ok());
  CHECK(1 == res.members().size());

  std::string member_name = res.members()[0].get_name();
  std::string prev_peer_urls = res.members()[0].get_peerURLs()[0];

  // Add a new member
  std::string peer_urls = "http://127.0.0.1:33691";
  std::string client_urls = "http://127.0.0.1:33690";
  bool is_learner = false;
  res = etcd.add_member(peer_urls, is_learner).get();
  REQUIRE(res.is_ok());

  // Create the directory for the new etcd server
  std::string cmd = "mkdir -p /tmp/new_etcd_member";
  system(cmd.c_str());

  // Start a new etcd server
  std::vector<std::string> args = {
      "--name",
      "new_etcd_member",
      "--initial-advertise-peer-urls",
      peer_urls,
      "--listen-peer-urls",
      peer_urls,
      "--initial-cluster",
      member_name + "=" + prev_peer_urls + ",new_etcd_member=" + peer_urls,
      "--initial-cluster-state",
      "existing",
      "--listen-client-urls",
      client_urls,
      "--advertise-client-urls",
      client_urls,
      "--data-dir",
      "/tmp/new_etcd_member",
  };

  new_etcd_pid = start_etcd_server("/usr/local/bin/etcd", args);

  std::this_thread::sleep_for(std::chrono::seconds(30));

  // Check the member's number
  {
    etcd::Response res = etcd.list_member().get();
    REQUIRE(res.is_ok());
    CHECK(2 == res.members().size());
  }
}

TEST_CASE("member remove") {
  etcd::Client etcd(etcd_url);
  etcd::Response res = etcd.list_member().get();
  REQUIRE(res.is_ok());
  CHECK(2 == res.members().size());

  uint64_t member_id = 0;
  for (const auto& member : res.members()) {
    if (member.get_name() == "new_etcd_member") {
      member_id = member.get_id();
      break;
    }
  }

  REQUIRE(member_id != 0);
  // Remove the new member
  res = etcd.remove_member(member_id).get();

  std::this_thread::sleep_for(std::chrono::seconds(30));

  // Check whether the new etcd server is quited
  {
    int status;
    pid_t wpid = waitpid(new_etcd_pid, &status, WNOHANG);
    REQUIRE(wpid == new_etcd_pid);
    REQUIRE(WIFEXITED(status));
    REQUIRE(WEXITSTATUS(status) == 0);
  }

  // Remove the directory for the new etcd server
  std::string cmd = "rm -rf /tmp/new_etcd_member";
  system(cmd.c_str());

  // Check the member's number
  {
    etcd::Response res = etcd.list_member().get();
    REQUIRE(res.is_ok());
    CHECK(1 == res.members().size());
  }
}
