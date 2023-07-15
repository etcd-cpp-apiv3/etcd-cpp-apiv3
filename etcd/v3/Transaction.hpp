#ifndef V3_SRC_TRANSACTION_HPP_
#define V3_SRC_TRANSACTION_HPP_

#include <memory>
#include <string>

namespace etcdserverpb {
class TxnRequest;
}

namespace etcdv3 {

enum class CompareResult {
  EQUAL = 0,
  GREATER = 1,
  LESS = 2,
  NOT_EQUAL = 3,
};

enum class CompareTarget {
  VERSION = 0,
  CREATE = 1,
  MOD = 2,
  VALUE = 3,
  LEASE = 4,
};

class Transaction {
 public:
  Transaction();
  ~Transaction();

  union Value {
    int64_t version;
    int64_t create_revision;
    int64_t mod_revision;
    std::string value;
    int64_t lease;
  };

  void add_compare(std::string const& key, CompareTarget const& target,
                   CompareResult const& result, Value const& target_value,
                   std::string const& range_end = "");

  void add_compare_version(std::string const& key, int64_t const& version,
                           std::string const& range_end = "");
  void add_compare_version(std::string const& key, CompareResult const& result,
                           int64_t const& version,
                           std::string const& range_end = "");
  void add_compare_create(std::string const& key,
                          int64_t const& create_revision,
                          std::string const& range_end = "");
  void add_compare_create(std::string const& key, CompareResult const& result,
                          int64_t const& create_revision,
                          std::string const& range_end = "");
  void add_compare_mod(std::string const& key, int64_t const& mod_revision,
                       std::string const& range_end = "");
  void add_compare_mod(std::string const& key, CompareResult const& result,
                       int64_t const& mod_revision,
                       std::string const& range_end = "");
  void add_compare_value(std::string const& key, std::string const& value,
                         std::string const& range_end = "");
  void add_compare_value(std::string const& key, CompareResult const& result,
                         std::string const& value,
                         std::string const& range_end = "");
  void add_compare_lease(std::string const& key, int64_t const& lease,
                         std::string const& range_end = "");
  void add_compare_lease(std::string const& key, CompareResult const& result,
                         int64_t const& lease,
                         std::string const& range_end = "");

  void add_success_range(std::string const& key,
                         std::string const& range_end = "",
                         bool const recursive = false, const int64_t limit = 0);
  void add_success_put(std::string const& key, std::string const& value,
                       int64_t const leaseid = 0, const bool prev_kv = false);
  void add_success_delete(std::string const& key,
                          std::string const& range_end = "",
                          bool const recursive = false,
                          const bool prev_kv = false);
  void add_success_txn(const std::shared_ptr<Transaction> txn);

  void add_failure_range(std::string const& key,
                         std::string const& range_end = "",
                         bool const recursive = false, const int64_t limit = 0);
  void add_failure_put(std::string const& key, std::string const& value,
                       int64_t const leaseid = 0, const bool prev_kv = false);
  void add_failure_delete(std::string const& key,
                          std::string const& range_end = "",
                          bool const recursive = false,
                          const bool prev_kv = false);
  void add_failure_txn(const std::shared_ptr<Transaction> txn);

  /**
   * @brief Compare, and create if succeed. If failed, the response will
   *        contains the previous value in "values()" field.
   */
  void setup_compare_and_create(std::string const& key,
                                std::string const& prev_value,
                                std::string const& create_key,
                                std::string const& value,
                                int64_t const leaseid = 0);

  /**
   * @brief Compare, or create if failed. If failed, the response will contains
   *        the previous value in "values()" field.
   */
  void setup_compare_or_create(std::string const& key,
                               std::string const& prev_value,
                               std::string const& create_key,
                               std::string const& value,
                               int64_t const leaseid = 0);

  /**
   * @brief Compare, and swap if succeed. If failed, the response will contains
   *        the previous value in "values()" field.
   */
  void setup_compare_and_swap(std::string const& key,
                              std::string const& prev_value,
                              std::string const& value,
                              int64_t const leaseid = 0);

  /**
   * @brief Compare, and swap if failed. If failed, the response will contains
   *        the previous value in "values()" field.
   */
  void setup_compare_or_swap(std::string const& key,
                             std::string const& prev_value,
                             std::string const& value,
                             int64_t const leaseid = 0);

  /**
   * @brief Compare, and delete if succeed. If failed, the response will
   *        contains the previous value in "values()" field.
   */
  void setup_compare_and_delete(std::string const& key,
                                std::string const& prev_value,
                                std::string const& delete_key,
                                std::string const& range_end = "",
                                const bool recursive = false);

  /**
   * @brief Compare, or delete if failed. If failed, the response will contains
   * the previous value in "values()" field.
   */
  void setup_compare_or_delete(std::string const& key,
                               std::string const& prev_value,
                               std::string const& delete_key,
                               std::string const& range_end = "",
                               const bool recursive = false);

  /**
   * @brief Compare, and create if succeed. If failed, the response will
   *        contains the previous value in "values()" field.
   */
  void setup_compare_and_create(std::string const& key,
                                const int64_t prev_revision,
                                std::string const& create_key,
                                std::string const& value,
                                int64_t const leaseid = 0);

  /**
   * @brief Compare, or create if failed. If failed, the response will contains
   *        the previous value in "values()" field.
   */
  void setup_compare_or_create(std::string const& key,
                               const int64_t prev_revision,
                               std::string const& create_key,
                               std::string const& value,
                               int64_t const leaseid = 0);

  /**
   * @brief Compare, and swap if succeed. If failed, the response will contains
   *        the previous value in "values()" field.
   */
  void setup_compare_and_swap(std::string const& key,
                              const int64_t prev_revision,
                              std::string const& value,
                              int64_t const leaseid = 0);

  /**
   * @brief Compare, and swap if failed. If failed, the response will contains
   *        the previous value in "values()" field.
   */
  void setup_compare_or_swap(std::string const& key,
                             const int64_t prev_revision,
                             std::string const& value,
                             int64_t const leaseid = 0);

  /**
   * @brief Compare, and delete if succeed. If failed, the response will
   *        contains the previous value in "values()" field.
   */
  void setup_compare_and_delete(std::string const& key,
                                const int64_t prev_revision,
                                std::string const& delete_key,
                                std::string const& range_end = "",
                                const bool recursive = false);

  /**
   * @brief Compare, or delete if failed. If failed, the response will contains
   * the previous value in "values()" field.
   */
  void setup_compare_or_delete(std::string const& key,
                               const int64_t prev_revision,
                               std::string const& delete_key,
                               std::string const& range_end = "",
                               const bool recursive = false);

  // Keep for backwards compatibility.

  // update without `get` and no `prev_kv` returned
  void setup_put(std::string const& key, std::string const& value);
  void setup_delete(std::string const& key);
  void setup_delete(std::string const& key, std::string const& range_end,
                    const bool recursive = false);

  std::shared_ptr<etcdserverpb::TxnRequest> txn_request;
};

}  // namespace etcdv3

#endif
