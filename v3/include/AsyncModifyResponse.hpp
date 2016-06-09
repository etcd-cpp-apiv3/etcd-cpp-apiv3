/*
 * AsyncModifyResponse.h
 *
 *  Created on: Jun 9, 2016
 *      Author: ubuntu
 */

#ifndef V3_SRC_ASYNCMODIFYRESPONSE_HPP_
#define V3_SRC_ASYNCMODIFYRESPONSE_HPP_

#include "v3/include/V3Response.hpp"
#include "v3/include/V3BaseResponse.hpp"
#include "v3/include/grpcClient.hpp"

namespace etcdv3 {

class AsyncModifyResponse : public etcdv3::V3Response, public etcdv3::V3BaseResponse {
public:
	AsyncModifyResponse(){action="compareAndSwap";};
	AsyncModifyResponse(std::string const &);
	AsyncModifyResponse(const AsyncModifyResponse&);
	AsyncModifyResponse& operator=(const AsyncModifyResponse&);
	virtual ~AsyncModifyResponse();

	etcdserverpb::PutResponse putResponse;
	std::unique_ptr<grpc::ClientAsyncResponseReader<etcdserverpb::PutResponse>> rpcInstance;
	AsyncModifyResponse& ParseResponse();
	etcdv3::grpcClient* client;
	std::string key;
};
} /* namespace etcdv3 */

#endif /* V3_SRC_ASYNCMODIFYRESPONSE_HPP_ */
