/*
 * BaseResponse.h
 *
 *  Created on: Jun 7, 2016
 *      Author: ubuntu
 */

#ifndef SRC_BASERESPONSE_H_
#define SRC_BASERESPONSE_H_

#include "etcd/Response.hpp"
#include "proto/rpc.grpc.pb.h"

namespace etcd {
class BaseResponse : public etcd::Response {
public:
	//TODO: turn into a abstract class
	BaseResponse();
	virtual ~BaseResponse();

	grpc::Status status;
	grpc::ClientContext context;
	grpc::CompletionQueue cq_;
};
}
#endif /* SRC_BASERESPONSE_H_ */
