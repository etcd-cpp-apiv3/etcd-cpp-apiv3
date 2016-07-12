#ifndef V3_SRC_TRANSACTION_HPP_
#define V3_SRC_TRANSACTION_HPP_

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"

#include <string>

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
        void setup_lease_grant_operation(int ttl);

	etcdserverpb::TxnRequest txn_request;
        etcdserverpb::LeaseGrantRequest leasegrant_request;

private:
	std::string key;
};

}

#endif
