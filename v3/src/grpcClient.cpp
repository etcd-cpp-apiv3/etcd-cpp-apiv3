#include "v3/include/grpcClient.hpp"

etcdv3::grpcClient::grpcClient(std::string const & address)
{
  std::string stripped_address(address);
  std::string substr("http://");
  std::string::size_type i = stripped_address.find(substr);
  if(i != std::string::npos)
  {
    stripped_address.erase(i,substr.length());
  }
  std::shared_ptr<Channel> channel = grpc::CreateChannel(stripped_address, grpc::InsecureChannelCredentials());
  stub_= KV::NewStub(channel);
}
