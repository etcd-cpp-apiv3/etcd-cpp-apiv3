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
    int error_code;
    int index;
    bool isPrefix;
    std::string error_message;
    std::string action;
    std::vector<mvccpb::KeyValue> values;
    std::vector<mvccpb::KeyValue> prev_values; 
  };
}
#endif
