#include "etcd/Watcher.hpp"
#include "etcd/v3/AsyncWatchAction.hpp"

etcd::Watcher::Watcher(Client const &client, std::string const & key,
                       std::function<void(Response)> callback, bool recursive):
    Watcher(client, key, -1, callback, recursive) {
}

etcd::Watcher::Watcher(Client const &client, std::string const & key, int fromIndex,
                       std::function<void(Response)> callback, bool recursive):
    fromIndex(fromIndex), recursive(recursive) {
  watchServiceStub= Watch::NewStub(client.channel);
  doWatch(key, client.auth_token, callback);
}

etcd::Watcher::Watcher(std::string const & address, std::string const & key,
                       std::function<void(Response)> callback, bool recursive):
    Watcher(address, key, -1, callback, recursive) {
}

etcd::Watcher::Watcher(std::string const & address, std::string const & key, int fromIndex,
                       std::function<void(Response)> callback, bool recursive):
    Watcher(Client(address), key, fromIndex, callback, recursive) {
}

etcd::Watcher::Watcher(std::string const & address,
            std::string const & username, std::string const & password,
            std::string const & key,
            std::function<void(Response)> callback, bool recursive):
    Watcher(address, username, password, key, -1, callback, recursive) {
}

etcd::Watcher::Watcher(std::string const & address,
            std::string const & username, std::string const & password,
            std::string const & key, int fromIndex,
            std::function<void(Response)> callback, bool recursive):
    Watcher(Client(address, username, password), key, fromIndex, callback, recursive) {
}

etcd::Watcher::~Watcher()
{
  call->CancelWatch();
  currentTask.wait();
}

bool etcd::Watcher::Wait()
{
  currentTask.wait();
  return call->Cancelled();
}

void etcd::Watcher::Wait(std::function<void(bool)> callback)
{
  currentTask.then([this, callback](pplx::task<void> const & resp_task) {
    resp_task.wait();
    callback(this->call->Cancelled());
  });
}

void etcd::Watcher::Cancel()
{
  call->CancelWatch();
  this->Wait();
}

void etcd::Watcher::doWatch(std::string const & key,
                            std::string const & auth_token,
                            std::function<void(Response)> callback)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(auth_token);
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
