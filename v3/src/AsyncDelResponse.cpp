/*
 * AsyncDelResponse.cpp
 *
 *  Created on: Jun 8, 2016
 *      Author: ubuntu
 */

#include "v3/include/AsyncDelResponse.hpp"
#include "v3/include/Utils.hpp"

etcdv3::AsyncDelResponse::AsyncDelResponse(std::string const &inputAction) {
	action = inputAction;
}

etcdv3::AsyncDelResponse::AsyncDelResponse(const etcdv3::AsyncDelResponse& other)
{
	error_code = other.error_code;
	error_message = other.error_message;
	index = other.index;
	action = other.action;
	values = other.values;
}

etcdv3::AsyncDelResponse& etcdv3::AsyncDelResponse::operator=(const etcdv3::AsyncDelResponse& other){
	error_code = other.error_code;
	error_message = other.error_message;
	index = other.index;
	action = other.action;
	values = other.values;
	return *this;
}

etcdv3::AsyncDelResponse::~AsyncDelResponse(){
}


//TODO: unused
etcdv3::AsyncDelResponse& etcdv3::AsyncDelResponse::ParseResponse(){
	action = "delete";

//	etcdv3::AsyncRangeResponse* resp = etcdv3::Utils::getKey(key, *client);
//	if(resp->reply.kvs_size())
//	{
//		values.push_back(resp->reply.kvs(0));
//	}

	return *this;
}
