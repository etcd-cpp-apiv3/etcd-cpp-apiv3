#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include "v3/include/AsyncRangeResponse.hpp"
#include "v3/include/grpcClient.hpp"

namespace etcdv3
{
  namespace Utils
  {
    etcdv3::AsyncRangeResponse* getKey(std::string const & key, etcdv3::grpcClient& client);
  }
}
#endif

