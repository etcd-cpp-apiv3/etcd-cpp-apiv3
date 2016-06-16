/*
 * AsyncModifyResponse.cpp
 *
 *  Created on: Jun 9, 2016
 *      Author: ubuntu
 */

#include "v3/include/AsyncModifyResponse.hpp"
#include "v3/include/Utils.hpp"

namespace etcdv3 {

etcdv3::AsyncModifyResponse::AsyncModifyResponse(const etcdv3::AsyncModifyResponse& other) {
	error_code = other.error_code;
	error_message = other.error_message;
	index = other.index;
	action = other.action;
	values = other.values;
        prev_values= other.prev_values;
}

etcdv3::AsyncModifyResponse::AsyncModifyResponse(const std::string &input) {
	action = input;
}

etcdv3::AsyncModifyResponse& etcdv3::AsyncModifyResponse::operator=(const etcdv3::AsyncModifyResponse& other) {
	  error_code = other.error_code;
	  error_message = other.error_message;
	  index = other.index;
	  action = other.action;
	  values = other.values;
          prev_values= other.prev_values;
	  return *this;
}

AsyncModifyResponse::~AsyncModifyResponse() {
	// TODO Auto-generated destructor stub
}

etcdv3::AsyncModifyResponse& etcdv3::AsyncModifyResponse::ParseResponse() {
	etcdv3::AsyncRangeResponse* response = etcdv3::Utils::getKey(key, *client);
	if(response->reply.kvs_size())
	{
		values.push_back(response->reply.kvs(0));
		index = response->reply.kvs(0).mod_revision();
	}
	else{
		index = response->reply.header().revision();
	}

	return *this;
}

} /* namespace etcdv3 */
