#ifndef __V3_RESPONSE_HPP__
#define __V3_RESPONSE_HPP__

#include "proto/kv.pb.h"


namespace etcdv3
{
  class V3Response
  {
    public:
    V3Response(): error_code(0), index(00) 
    {
        prev_value.set_key("");
        prev_value.set_create_revision(0);
        prev_value.set_mod_revision(0);
        prev_value.set_value("");
    };
    int error_code;
    std::string error_message;
    int index;
    std::string action;
    std::vector<mvccpb::KeyValue> values;
    mvccpb::KeyValue prev_value;
  };
}
#endif
