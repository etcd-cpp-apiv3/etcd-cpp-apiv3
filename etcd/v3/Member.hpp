#ifndef __V3_ETCDV3MEMBERS_HPP__
#define __V3_ETCDV3MEMBERS_HPP__

#include <string>
#include <vector>
using namespace std;

namespace etcdv3 {
class Member {
 public:
  Member();

  void set_id(uint64_t const& id);
  uint64_t const& get_id() const;
  void set_name(std::string const& name);
  std::string const& get_name() const;
  void set_peerURLs(std::vector<std::string> const& peerURLs);
  std::vector<std::string> const& get_peerURLs() const;
  void set_clientURLs(std::vector<std::string> const& clientURLs);
  std::vector<std::string> const& get_clientURLs() const;
  void set_learner(bool isLearner);
  bool get_learner() const;

 private:
  uint64_t id;
  std::string name;
  std::vector<std::string> peerURLs;
  std::vector<std::string> clientURLs;
  bool isLearner;
};
}  // namespace etcdv3
#endif
