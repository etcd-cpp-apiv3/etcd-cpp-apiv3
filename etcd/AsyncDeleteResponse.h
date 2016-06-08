/*
 * AsyncDeleteResponse.h
 *
 *  Created on: Jun 7, 2016
 *      Author: ubuntu
 */

#ifndef SRC_ASYNCDELETERESPONSE_H_
#define SRC_ASYNCDELETERESPONSE_H_

#include "etcd/BaseResponse.h"

namespace etcd {
class AsyncDeleteResponse : public etcd::BaseResponse {
public:
	AsyncDeleteResponse();
	virtual ~AsyncDeleteResponse();

	void fillUpV2ResponseValues(etcdserverpb::RangeResponse);
	etcdserverpb::DeleteRangeResponse deleteResponse;

	std::unique_ptr<grpc::ClientAsyncResponseReader<etcdserverpb::DeleteRangeResponse>> rpcInstance;

};
}
#endif /* SRC_ASYNCDELETERESPONSE_H_ */
