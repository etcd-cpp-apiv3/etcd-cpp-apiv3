#ifndef __V3_RESPONSE_HPP__
#define __V3_RESPONSE_HPP__

#include <grpc++/grpc++.h>
#include "proto/kv.pb.h"

namespace etcdv3
{
  class V3Response
  {
  public:
    V3Response(): error_code(0), index(0), isPrefix(false) {};
    void set_error_code(int code);
    int get_error_code() const;
    std::string const & get_error_message() const;
    void set_error_message(std::string msg);
    void set_action(std::string action);
    int get_index() const;
    std::string const & get_action() const;
    std::vector<mvccpb::KeyValue> const & get_values() const;
    std::vector<mvccpb::KeyValue> const & get_prev_values() const;
    mvccpb::KeyValue const & get_value() const;
    mvccpb::KeyValue const & get_prev_value() const;
    bool has_values() const;
  protected:
    int error_code;
    int index;
    bool isPrefix;
    std::string error_message;
    std::string action;
    mvccpb::KeyValue value;
    mvccpb::KeyValue prev_value; 
    std::vector<mvccpb::KeyValue> values;
    std::vector<mvccpb::KeyValue> prev_values; 
  };
}
#endif
