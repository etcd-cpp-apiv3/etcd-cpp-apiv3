/*
 * AsyncDelResponse.h
 *
 *  Created on: Jun 8, 2016
 *      Author: ubuntu
 */

#ifndef V3_SRC_ASYNCDELRESPONSE_HPP_
#define V3_SRC_ASYNCDELRESPONSE_HPP_

#include "v3/include/V3Response.hpp"
#include "v3/include/V3BaseResponse.hpp"
#include "v3/include/grpcClient.hpp"

namespace etcdv3 {

class AsyncDelResponse : public etcdv3::V3Response, public etcdv3::V3BaseResponse {
public:
	AsyncDelResponse(){action="delete";};
	AsyncDelResponse(std::string const &);
	AsyncDelResponse(const AsyncDelResponse&);
	AsyncDelResponse& operator=(const AsyncDelResponse&);
	virtual ~AsyncDelResponse();

	etcdserverpb::DeleteRangeResponse deleteResponse;
	std::unique_ptr<grpc::ClientAsyncResponseReader<etcdserverpb::DeleteRangeResponse>> rpcInstance;
	AsyncDelResponse& ParseResponse();
	etcdv3::grpcClient* client;
	std::string key;
};
} /* namespace etcdv3 */

#endif /* V3_SRC_ASYNCDELRESPONSE_HPP_ */
