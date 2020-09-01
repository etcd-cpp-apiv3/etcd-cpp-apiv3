#include "etcd/Watcher.hpp"
#include "etcd/v3/AsyncWatchAction.hpp"

etcd::Watcher::Watcher(Client &client, std::string const & key,
                       std::function<void(Response)> callback, bool recursive):
    Watcher(client, key, -1, callback, recursive) {
}


etcd::Watcher::Watcher(Client &client, std::string const & key, int fromIndex,
                       std::function<void(Response)> callback, bool recursive):
    fromIndex(fromIndex), recursive(recursive) {
  watchServiceStub= Watch::NewStub(client.channel);

  doWatch(key, callback);
}

etcd::Watcher::Watcher(std::string const & address, std::string const & key,
                       std::function<void(Response)> callback, bool recursive):
    Watcher(address, key, -1, callback, recursive) {
}

etcd::Watcher::Watcher(std::string const & address, std::string const & key, int fromIndex,
                       std::function<void(Response)> callback, bool recursive):
    fromIndex(fromIndex), recursive(recursive) {
  std::string stripped_address(address);
  std::string substr("http://");
  std::string::size_type i = stripped_address.find(substr);
  if(i != std::string::npos)
  {
    stripped_address.erase(i,substr.length());
  }

  std::shared_ptr<Channel> channel = grpc::CreateChannel(
      stripped_address, grpc::InsecureChannelCredentials());
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
  if (fromIndex >= 0) {
    params.revision = fromIndex;
  }
  params.withPrefix = recursive;
  params.watch_stub = watchServiceStub.get();

  call.reset(new etcdv3::AsyncWatchAction(params));

  currentTask = pplx::task<void>([this, callback]()
  {  
    return call->waitForResponse(callback);
  });
}
