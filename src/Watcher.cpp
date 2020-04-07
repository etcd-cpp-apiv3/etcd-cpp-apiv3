#include "etcd/Watcher.hpp"
#include "v3/include/AsyncWatchAction.hpp"

etcd::Watcher::Watcher(std::string const & address, std::string const & key, std::function<void(Response)> callback)
{
  std::string stripped_address(address);
  std::string substr("http://");
  std::string::size_type i = stripped_address.find(substr);
  if(i != std::string::npos)
  {
    stripped_address.erase(i,substr.length());
  }
  std::shared_ptr<Channel> channel = grpc::CreateChannel(stripped_address, grpc::InsecureChannelCredentials());
  watchServiceStub= Watch::NewStub(channel);

  doWatch(key, callback);
}


etcd::Watcher::~Watcher()
{
  call->CancelWatch();
  currentTask.wait();
}

void etcd::Watcher::Cancel()
{
  call->CancelWatch();
  currentTask.wait();
}

void etcd::Watcher::doWatch(std::string const & key, std::function<void(Response)> callback)
{
  etcdv3::ActionParameters params;
  params.key.assign(key);
  params.withPrefix = true;
  params.watch_stub = watchServiceStub.get();
  params.revision = 0;
  call.reset(new etcdv3::AsyncWatchAction(params));

  currentTask = pplx::task<void>([this, callback]()
  {  
    return call->waitForResponse(callback);
  });
}
