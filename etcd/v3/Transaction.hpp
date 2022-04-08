#ifndef V3_SRC_TRANSACTION_HPP_
#define V3_SRC_TRANSACTION_HPP_

#include <string>
#include <memory>

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
    Transaction(std::string const&);
    virtual ~Transaction();

    // Set a new key for different comparisons and /put/get/delete requests.
    void reset_key(std::string const& newkey);

    void init_compare(CompareResult, CompareTarget);
    void init_compare(std::string const &old_value, CompareResult, CompareTarget);
    void init_compare(int64_t old_value, CompareResult, CompareTarget);

    void setup_basic_failure_operation(std::string const &key);
    void setup_set_failure_operation(std::string const &key, std::string const &value, int64_t leaseid);
    void setup_basic_create_sequence(std::string const &key, std::string const &value, int64_t leaseid);
    void setup_compare_and_swap_sequence(std::string const &valueToSwap, int64_t leaseid);
    void setup_delete_sequence(std::string const &key, std::string const &range_end, bool recursive);
    void setup_delete_failure_operation(std::string const &key, std::string const &range_end, bool recursive);
    void setup_compare_and_delete_operation(std::string const& key);

    // update without `get` and no `prev_kv` returned
    void setup_put(std::string const &key, std::string const &value);
    void setup_delete(std::string const &key);

    std::unique_ptr<etcdserverpb::TxnRequest> txn_request;
  private:
    std::string key;
};

}

#endif
