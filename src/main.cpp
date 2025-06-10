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

  std::this_thread::sleep_for(std::chrono::seconds(1));
  return 0;
}
