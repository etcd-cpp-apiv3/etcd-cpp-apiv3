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
    void set_error_message(std::string msg);
    void set_action(std::string action);
    std::vector<mvccpb::KeyValue> get_kv_values();
    std::vector<mvccpb::KeyValue> get_prev_kv_values();
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
