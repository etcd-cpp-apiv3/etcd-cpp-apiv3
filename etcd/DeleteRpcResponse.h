/*
 * DeleteRpcResponse.h
 *
 *  Created on: Jun 5, 2016
 *      Author: ubuntu
 */

//#ifndef SRC_DELETERPCRESPONSE_H_
//#define SRC_DELETERPCRESPONSE_H_
//
//#include "etcd/Response.hpp"
//#include "proto/rpc.grpc.pb.h"
//
//namespace etcd {
//class DeleteRpcResponse : public etcd::Response, public etcd::Value {
//public:
//	DeleteRpcResponse();
//	virtual ~DeleteRpcResponse();
//	void fillUpV2ResponseValues(etcdserverpb::RangeResponse);
//
//	etcdserverpb::DeleteRangeResponse deleteResponse;
//
//	grpc::Status status;
//	grpc::ClientContext context;
//	grpc::CompletionQueue cq_;
//
//	std::unique_ptr<grpc::ClientAsyncResponseReader<etcdserverpb::DeleteRangeResponse>> rpcInstance;
//};
//}
//#endif /* SRC_DELETERPCRESPONSE_H_ */
