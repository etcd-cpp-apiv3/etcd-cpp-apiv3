// See also: https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3/issues/86

#include <future>
#include <memory>

#include "etcd/Client.hpp"
#include "etcd/KeepAlive.hpp"

class DistributedLock {
public:
    DistributedLock(const std::string &lock_name,
                    uint timeout = 0);
    ~DistributedLock() noexcept;
    inline bool lock_acquired() {
      return _acquired;
    }

private:
    bool _acquired = false;
    std::string _lock_key;
    std::unique_ptr<::etcd::Client> _etcd_client;
};

DistributedLock::DistributedLock(const std::string &lock_name,
                                 uint timeout) {
  _etcd_client = std::unique_ptr<etcd::Client>(new etcd::Client("localhost:2379"));

  try {
    if (timeout == 0) {
      etcd::Response resp = _etcd_client->lock(lock_name).get();
      if (resp.is_ok()) {
        _lock_key = resp.lock_key();
        _acquired = true;
      }
    } else {
      std::future<etcd::Response> future = std::async(std::launch::async, [&]() {
          etcd::Response resp = _etcd_client->lock(lock_name).get();
          return resp;
      });

      std::future_status status = future.wait_for(std::chrono::seconds(timeout));
      if (status == std::future_status::ready) {
        auto resp = future.get();
        if (resp.is_ok()) {
          _lock_key = resp.lock_key();
          _acquired = true;
        }
      } else if (status == std::future_status::timeout) {
        std::cerr << "failed to acquire distributed because of lock timeout" << std::endl;
      } else {
        std::cerr << "failed to acquire distributed lock" << std::endl;
      }
    }
  } catch (std::exception &e) {
    std::cerr << "failed to construct: " << e.what() << std::endl;
  }
}

DistributedLock::~DistributedLock() noexcept {
  if (!_acquired) {
    return;
  }

  try {
    auto resp = _etcd_client->unlock(_lock_key).get();
    if (!resp.is_ok()) {
      std::cout << resp.error_code() << std::endl;
    }
  } catch (std::exception &e) {
    std::cerr << "failed to destruct: " << e.what() << std::endl;
  }
}

int main() {
  int i = 0, t = 0;
  while(t < 10 /* update this value to make it run for longer */) {
    {
      DistributedLock lock(std::to_string(i), 0);
      if(!lock.lock_acquired()) {
        std::cerr << "failed to acquire lock" << std::endl;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    ++i;
    ++t;
    if (i == 10) {
      i = 0;
    }
    std::cout << "round: i = " << i << ", t = " << t << std::endl;
  }
}
