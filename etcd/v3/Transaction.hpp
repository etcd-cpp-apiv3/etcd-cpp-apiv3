#ifndef V3_SRC_TRANSACTION_HPP_
#define V3_SRC_TRANSACTION_HPP_

#include <string>

#include "txn.pb.h"

namespace etcdserverpb {
	class TxnRequest;
}

namespace etcdv3 {

class Transaction {
public:
	Transaction();
	Transaction(std::string const&);
	virtual ~Transaction();
	void init_compare(etcdserverpb::Compare::CompareResult, etcdserverpb::Compare::CompareTarget);
	void init_compare(std::string const &, etcdserverpb::Compare::CompareResult, etcdserverpb::Compare::CompareTarget);
	void init_compare(int, etcdserverpb::Compare::CompareResult, etcdserverpb::Compare::CompareTarget);

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
