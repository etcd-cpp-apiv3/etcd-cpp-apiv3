#include "etcd/Watcher.hpp"

#include "etcd/SyncClient.hpp"

#include "etcd/v3/AsyncGRPC.hpp"

struct etcd::Watcher::EtcdServerStubs {
  std::unique_ptr<Watch::Stub> watchServiceStub;
  std::unique_ptr<etcdv3::AsyncWatchAction> call;
};

void etcd::Watcher::EtcdServerStubsDeleter::operator()(
    etcd::Watcher::EtcdServerStubs* stubs) {
  if (stubs) {
    if (stubs->watchServiceStub) {
      stubs->watchServiceStub.reset();
    }
    if (stubs->call) {
      stubs->call.reset();
    }
    delete stubs;
  }
}

etcd::Watcher::Watcher(SyncClient const& client, std::string const& key,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback, bool recursive)
    : Watcher(client, key, -1, callback, wait_callback, recursive) {}

etcd::Watcher::Watcher(SyncClient const& client, std::string const& key,
                       std::string const& range_end,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback)
    : Watcher(client, key, range_end, -1, callback, wait_callback) {}

etcd::Watcher::Watcher(SyncClient const& client, std::string const& key,
                       int64_t fromIndex,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback, bool recursive)
    : wait_callback(wait_callback), fromIndex(fromIndex), recursive(recursive) {
  stubs.reset(new EtcdServerStubs{});
  stubs->watchServiceStub = Watch::NewStub(client.channel);
  doWatch(key, "", client.current_auth_token(), callback);
}

etcd::Watcher::Watcher(SyncClient const& client, std::string const& key,
                       std::string const& range_end, int64_t fromIndex,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback)
    : wait_callback(wait_callback), fromIndex(fromIndex), recursive(false) {
  stubs.reset(new EtcdServerStubs{});
  stubs->watchServiceStub = Watch::NewStub(client.channel);
  doWatch(key, range_end, client.current_auth_token(), callback);
}

etcd::Watcher::Watcher(std::string const& address, std::string const& key,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback, bool recursive)
    : Watcher(address, key, -1, callback, wait_callback, recursive) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& key,
                       std::string const& range_end,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback)
    : Watcher(address, key, range_end, -1, callback, wait_callback) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& key,
                       int64_t fromIndex,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback, bool recursive)
    : Watcher(SyncClient(address), key, fromIndex, callback, wait_callback,
              recursive) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& key,
                       std::string const& range_end, int64_t fromIndex,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback)
    : Watcher(SyncClient(address), key, range_end, fromIndex, callback,
              wait_callback) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& username,
                       std::string const& password, std::string const& key,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback, bool recursive,
                       int const auth_token_ttl)
    : Watcher(address, username, password, key, -1, callback, wait_callback,
              recursive, auth_token_ttl) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& username,
                       std::string const& password, std::string const& key,
                       std::string const& range_end,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback,
                       int const auth_token_ttl)
    : Watcher(address, username, password, key, range_end, -1, callback,
              wait_callback, auth_token_ttl) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& username,
                       std::string const& password, std::string const& key,
                       int64_t fromIndex,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback, bool recursive,
                       int const auth_token_ttl)
    : Watcher(SyncClient(address, username, password, auth_token_ttl), key,
              fromIndex, callback, wait_callback, recursive) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& username,
                       std::string const& password, std::string const& key,
                       std::string const& range_end, int64_t fromIndex,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback,
                       int const auth_token_ttl)
    : Watcher(SyncClient(address, username, password, auth_token_ttl), key,
              range_end, fromIndex, callback, wait_callback) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& ca,
                       std::string const& cert, std::string const& privkey,
                       std::string const& key, int64_t fromIndex,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback, bool recursive,
                       std::string const& target_name_override)
    : Watcher(SyncClient(address, ca, cert, privkey, target_name_override), key,
              fromIndex, callback, wait_callback, recursive) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& ca,
                       std::string const& cert, std::string const& privkey,
                       std::string const& key, std::string const& range_end,
                       int64_t fromIndex,
                       std::function<void(Response)> callback,
                       std::function<void(bool)> wait_callback,
                       std::string const& target_name_override)
    : Watcher(SyncClient(address, ca, cert, privkey, target_name_override), key,
              range_end, fromIndex, callback, wait_callback) {}

etcd::Watcher::Watcher(SyncClient const& client, std::string const& key,
                       std::function<void(Response)> callback, bool recursive)
    : Watcher(client, key, callback, nullptr, recursive) {}

etcd::Watcher::Watcher(SyncClient const& client, std::string const& key,
                       std::string const& range_end,
                       std::function<void(Response)> callback)
    : Watcher(client, key, range_end, callback, nullptr) {}

etcd::Watcher::Watcher(SyncClient const& client, std::string const& key,
                       int64_t fromIndex,
                       std::function<void(Response)> callback, bool recursive)
    : Watcher(client, key, fromIndex, callback, nullptr, recursive) {}

etcd::Watcher::Watcher(SyncClient const& client, std::string const& key,
                       std::string const& range_end, int64_t fromIndex,
                       std::function<void(Response)> callback)
    : Watcher(client, key, range_end, fromIndex, callback, nullptr) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& key,
                       std::function<void(Response)> callback, bool recursive)
    : Watcher(address, key, callback, nullptr, recursive) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& key,
                       std::string const& range_end,
                       std::function<void(Response)> callback)
    : Watcher(address, key, range_end, callback, nullptr) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& key,
                       int64_t fromIndex,
                       std::function<void(Response)> callback, bool recursive)
    : Watcher(address, key, fromIndex, callback, nullptr, recursive) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& key,
                       std::string const& range_end, int64_t fromIndex,
                       std::function<void(Response)> callback)
    : Watcher(address, key, range_end, fromIndex, callback, nullptr) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& username,
                       std::string const& password, std::string const& key,
                       std::function<void(Response)> callback, bool recursive,
                       int const auth_token_ttl)
    : Watcher(address, username, password, key, callback, nullptr, recursive,
              auth_token_ttl) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& username,
                       std::string const& password, std::string const& key,
                       std::string const& range_end,
                       std::function<void(Response)> callback,
                       int const auth_token_ttl)
    : Watcher(address, username, password, key, range_end, callback, nullptr,
              auth_token_ttl) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& username,
                       std::string const& password, std::string const& key,
                       int64_t fromIndex,
                       std::function<void(Response)> callback, bool recursive,
                       int const auth_token_ttl)
    : Watcher(address, username, password, key, fromIndex, callback, nullptr,
              recursive, auth_token_ttl) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& username,
                       std::string const& password, std::string const& key,
                       std::string const& range_end, int64_t fromIndex,
                       std::function<void(Response)> callback,
                       int const auth_token_ttl)
    : Watcher(address, username, password, key, range_end, fromIndex, callback,
              nullptr, auth_token_ttl) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& ca,
                       std::string const& cert, std::string const& privkey,
                       std::string const& key, int64_t fromIndex,
                       std::function<void(Response)> callback, bool recursive,
                       std::string const& target_name_override)
    : Watcher(address, ca, cert, privkey, key, fromIndex, callback, nullptr,
              recursive, target_name_override) {}

etcd::Watcher::Watcher(std::string const& address, std::string const& ca,
                       std::string const& cert, std::string const& privkey,
                       std::string const& key, std::string const& range_end,
                       int64_t fromIndex,
                       std::function<void(Response)> callback,
                       std::string const& target_name_override)
    : Watcher(address, ca, cert, privkey, key, range_end, fromIndex, callback,
              nullptr, target_name_override) {}

etcd::Watcher::~Watcher() { this->Cancel(); }

bool etcd::Watcher::Wait() {
  if (!cancelled.exchange(true)) {
    if (task_.joinable()) {
      task_.join();
    }
  }
  return stubs->call->Cancelled();
}

bool etcd::Watcher::Wait(std::function<void(bool)> callback) {
  if (wait_callback == nullptr) {
    wait_callback = callback;
    return true;
  } else {
#ifndef NDEBUG
    std::cerr
        << "[warn] failed to set a asynchronous wait callback since it has "
           "already been set"
        << std::endl;
#endif
    return false;
  }
}

bool etcd::Watcher::Cancel() {
  stubs->call->CancelWatch();
  return this->Wait();
}

bool etcd::Watcher::Cancelled() const {
  return cancelled.load() || stubs->call->Cancelled();
}

void etcd::Watcher::doWatch(std::string const& key,
                            std::string const& range_end,
                            std::string const& auth_token,
                            std::function<void(Response)> callback) {
  etcdv3::ActionParameters params;
  params.auth_token.assign(auth_token);
  // n.b.: watch: no need for timeout
  params.key.assign(key);
  params.range_end.assign(range_end);
  if (fromIndex >= 0) {
    params.revision = fromIndex;
  }
  params.withPrefix = recursive;
  params.watch_stub = stubs->watchServiceStub.get();

  stubs->call.reset(new etcdv3::AsyncWatchAction(std::move(params)));

  task_ = std::thread([this, callback]() {
    stubs->call->waitForResponse(callback);
    if (wait_callback != nullptr) {
      // issue the callback in another thread (detached) to avoid deadlock,
      // it is ok to detach a pplx::task, but we don't want to use the
      // pplx::task in the core library
      bool cancelled = stubs->call->Cancelled();
      std::function<void(bool)> wait_callback = this->wait_callback;
      std::thread canceller(
          [wait_callback, cancelled]() { wait_callback(cancelled); });
      canceller.detach();
    }
  });
  cancelled.store(false);
}
