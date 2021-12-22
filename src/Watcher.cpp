#include "etcd/Watcher.hpp"
#include "etcd/v3/AsyncWatchAction.hpp"

struct etcd::Watcher::EtcdServerStubs {
  std::unique_ptr<Watch::Stub> watchServiceStub;
  std::unique_ptr<etcdv3::AsyncWatchAction> call;
};

void etcd::Watcher::EtcdServerStubsDeleter::operator()(etcd::Watcher::EtcdServerStubs *stubs) {
  if (stubs) {
    delete stubs;
  }
}

etcd::Watcher::Watcher(Client const &client, std::string const & key,
                       std::function<void(Response)> callback, bool recursive):
    Watcher(client, key, -1, callback, recursive) {
}

etcd::Watcher::Watcher(Client const &client, std::string const & key,
                       std::string const &range_end,
                       std::function<void(Response)> callback):
    Watcher(client, key, range_end, -1, callback) {
}

etcd::Watcher::Watcher(Client const &client, std::string const & key, int fromIndex,
                       std::function<void(Response)> callback, bool recursive):
    fromIndex(fromIndex), recursive(recursive) {
  stubs.reset(new EtcdServerStubs{});
  stubs->watchServiceStub = Watch::NewStub(client.channel);
  doWatch(key, "", client.current_auth_token(), callback);
}

etcd::Watcher::Watcher(Client const &client, std::string const & key,
                       std::string const &range_end, int fromIndex,
                       std::function<void(Response)> callback):
    fromIndex(fromIndex), recursive(false) {
  stubs.reset(new EtcdServerStubs{});
  stubs->watchServiceStub = Watch::NewStub(client.channel);
  doWatch(key, range_end, client.current_auth_token(), callback);
}

etcd::Watcher::Watcher(std::string const & address, std::string const & key,
                       std::function<void(Response)> callback, bool recursive):
    Watcher(address, key, -1, callback, recursive) {
}

etcd::Watcher::Watcher(std::string const & address, std::string const & key,
                       std::string const & range_end,
                       std::function<void(Response)> callback):
    Watcher(address, key, range_end, -1, callback) {
}

etcd::Watcher::Watcher(std::string const & address, std::string const & key, int fromIndex,
                       std::function<void(Response)> callback, bool recursive):
    Watcher(Client(address), key, fromIndex, callback, recursive) {
}

etcd::Watcher::Watcher(std::string const & address, std::string const & key,
                       std::string const & range_end, int fromIndex,
                       std::function<void(Response)> callback):
    Watcher(Client(address), key, range_end, fromIndex, callback) {
}

etcd::Watcher::Watcher(std::string const & address,
            std::string const & username, std::string const & password,
            std::string const & key,
            std::function<void(Response)> callback, bool recursive,
            int const auth_token_ttl):
    Watcher(address, username, password, key, -1, callback, recursive, auth_token_ttl) {
}

etcd::Watcher::Watcher(std::string const & address,
            std::string const & username, std::string const & password,
            std::string const & key, std::string const & range_end,
            std::function<void(Response)> callback,
            int const auth_token_ttl):
    Watcher(address, username, password, key, range_end, -1, callback, auth_token_ttl) {
}

etcd::Watcher::Watcher(std::string const & address,
            std::string const & username, std::string const & password,
            std::string const & key, int fromIndex,
            std::function<void(Response)> callback, bool recursive,
            int const auth_token_ttl):
    Watcher(Client(address, username, password, auth_token_ttl), key, fromIndex, callback, recursive) {
}

etcd::Watcher::Watcher(std::string const & address,
            std::string const & username, std::string const & password,
            std::string const & key, std::string const & range_end, int fromIndex,
            std::function<void(Response)> callback,
            int const auth_token_ttl):
    Watcher(Client(address, username, password, auth_token_ttl), key, range_end, fromIndex, callback) {
}

etcd::Watcher::~Watcher()
{
  this->Cancel();
}

bool etcd::Watcher::Wait()
{
  if (!cancelled.exchange(true)) {
    if (task_.joinable()) {
      task_.join();
    }
  }
  return stubs->call->Cancelled();
}

void etcd::Watcher::Wait(std::function<void(bool)> callback)
{
  if (wait_callback == nullptr) {
    wait_callback = callback;
  } else {
    std::cerr << "Failed to set a asynchronous wait callback since it has already been set" << std::endl;
  }
}

void etcd::Watcher::Cancel()
{
  stubs->call->CancelWatch();
  this->Wait();
}

void etcd::Watcher::doWatch(std::string const & key,
                            std::string const & range_end,
                            std::string const & auth_token,
                            std::function<void(Response)> callback)
{
  etcdv3::ActionParameters params;
  params.auth_token.assign(auth_token);
  params.key.assign(key);
  params.range_end.assign(range_end);
  if (fromIndex >= 0) {
    params.revision = fromIndex;
  }
  params.withPrefix = recursive;
  params.watch_stub = stubs->watchServiceStub.get();

  stubs->call.reset(new etcdv3::AsyncWatchAction(params));

  task_ = std::thread([this, callback]() {
    stubs->call->waitForResponse(callback);
    if (wait_callback != nullptr) {
      wait_callback(stubs->call->Cancelled());
    }
  });
  cancelled.store(false);
}
