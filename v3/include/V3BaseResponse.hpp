/*
 * V3BaseResponse.h
 *
 *  Created on: Jun 8, 2016
 *      Author: ubuntu
 */

#ifndef V3_SRC_V3BASERESPONSE_H_
#define V3_SRC_V3BASERESPONSE_H_

#include <grpc++/grpc++.h>
#include "proto/rpc.grpc.pb.h"
#include "proto/kv.pb.h"

//TODO: make into abstract class
namespace etcdv3 {
class V3BaseResponse {
public:
	V3BaseResponse();
	virtual ~V3BaseResponse();

	grpc::Status status;
	grpc::ClientContext context;
	grpc::CompletionQueue cq_;
	// type& parseResponse()=0; //a possible candidate to make this abstract
};
} /* namespace etcdv3 */

#endif /* V3_SRC_V3BASERESPONSE_H_ */
