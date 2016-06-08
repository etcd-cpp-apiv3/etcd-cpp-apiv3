/*
 * AsyncDeleteResponse.cpp
 *
 *  Created on: Jun 7, 2016
 *      Author: ubuntu
 */

#include "etcd/AsyncDeleteResponse.h"

etcd::AsyncDeleteResponse::AsyncDeleteResponse() {
	// TODO Auto-generated constructor stub
}

etcd::AsyncDeleteResponse::~AsyncDeleteResponse() {
	// TODO Auto-generated destructor stub
}

void etcd::AsyncDeleteResponse::fillUpV2ResponseValues(etcdserverpb::RangeResponse getResponse) {
	_prev_value.value = getResponse.kvs().Get(0).value();
	_action = "delete";
}
