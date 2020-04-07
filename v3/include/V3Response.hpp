#ifndef __V3_RESPONSE_HPP__
#define __V3_RESPONSE_HPP__

#include <grpc++/grpc++.h>
#include "proto/kv.pb.h"

#include "v3/include/KeyValue.hpp"

namespace etcdv3
{
  class V3Response
  {
  public:
    V3Response(): error_code(0), index(0){};
    void set_error_code(int code);
    int get_error_code() const;
    std::string const & get_error_message() const;
    void set_error_message(std::string msg);
    void set_action(std::string action);
    int get_index() const;
    std::string const & get_action() const;
    std::vector<etcdv3::KeyValue> const & get_values() const;
    std::vector<etcdv3::KeyValue> const & get_prev_values() const;
    etcdv3::KeyValue const & get_value() const;
    etcdv3::KeyValue const & get_prev_value() const;
    bool has_values() const;
    void set_lock_key(std::string const &key);
    std::string const &get_lock_key() const;
    std::vector<mvccpb::Event> const & get_events() const;
  protected:
    int error_code;
    int index;
    std::string error_message;
    std::string action;
    etcdv3::KeyValue value;
    etcdv3::KeyValue prev_value; 
    std::vector<etcdv3::KeyValue> values;
    std::vector<etcdv3::KeyValue> prev_values; 
    std::string lock_key; // for lock
    std::vector<mvccpb::Event> events; // for watch
  };
}
#endif
