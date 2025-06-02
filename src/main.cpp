#include <etcd/SyncClient.hpp>
#include <etcd/Watcher.hpp>

int main() {
  auto etcd_client = etcd::SyncClient("http://127.0.0.1:2379");
  etcd_client.rmdir("/test", true);
  etcd_client.put("/test/0", "val_0");
  auto ls_resp = etcd_client.ls("/test");
  etcd_client.put("/test/1", "val_1");
  etcd_client.put("/test/2", "val_2");
  etcd_client.rm("/test/2");

  auto watcher = etcd::Watcher(
      etcd_client, "/test", ls_resp.index() + 1,
      [](const etcd::Response& resp) {
        auto actions = resp.actions();
        auto values = resp.values();
        std::cout << "size of values: " << values.size() << std::endl;
        std::cout << "size of keys: " << resp.keys().size() << std::endl;
        for (auto i = 0u; i < values.size(); ++i) {
          auto action = actions[i];
          auto val = values[i];
          std::cout << "action: " << action << ", key: " << val.key()
                    << ", val: " << val.as_string() << std::endl;
          // callback(
          //     response(resp.action(), val.key(), val.as_string()));
        }
      },
      true /*recursive*/);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  return 0;
}
