#include "etcd/v3/Member.hpp"

etcdv3::Member::Member() {
  id = 0;
  name = "";
  peerURLs = {};
  clientURLs = {};
  isLearner = false;
}

void etcdv3::Member::set_id(uint64_t const& id) { this->id = id; }

void etcdv3::Member::set_name(std::string const& name) { this->name = name; }

void etcdv3::Member::set_peerURLs(std::vector<std::string> const& peerURLs) {
  this->peerURLs = peerURLs;
}

void etcdv3::Member::set_clientURLs(
    std::vector<std::string> const& clientURLs) {
  this->clientURLs = clientURLs;
}

void etcdv3::Member::set_learner(bool isLearner) {
  this->isLearner = isLearner;
}

uint64_t const& etcdv3::Member::get_id() const { return id; }

std::string const& etcdv3::Member::get_name() const { return name; }

std::vector<std::string> const& etcdv3::Member::get_peerURLs() const {
  return peerURLs;
}

std::vector<std::string> const& etcdv3::Member::get_clientURLs() const {
  return clientURLs;
}

bool etcdv3::Member::get_learner() const { return isLearner; }
