/*
 * DeleteRpcResponse.cpp
 *
 *  Created on: Jun 5, 2016
 *      Author: ubuntu
 */

//#include "etcd/DeleteRpcResponse.h"
//#include <iostream>
//
//etcd::DeleteRpcResponse::DeleteRpcResponse() {
//	// TODO Auto-generated constructor stub
//
//}
//
//etcd::DeleteRpcResponse::~DeleteRpcResponse() {
//	// TODO Auto-generated destructor stub
//}
//
//void etcd::DeleteRpcResponse::fillUpV2ResponseValues(etcdserverpb::RangeResponse getResponse) {
//	std::cout << "inside fillup attempting fillupv2 response values" << std::endl;
//
//	std::cout << "setting the value of: " << getResponse.kvs().Get(0).value() << std::endl;
//	_prev_value.value = getResponse.kvs().Get(0).value();
//	_action = "delete";
//
//// //the experiments done on how to fill up the field of this object.
////	//access the fields na protected sa Value
////	this->Value::value = "yeah";
////	std::cout << "pre contents: " << this->_value.as_string() << std::endl;
////
////	//or create an instance here and return it filled up
////	DeleteRpcResponse r;
////	r.Value::value = "yo!";
////	this->_value = r; //strip off value part
////
////	std::cout << "contents: " << this->_value.as_string() << std::endl;
//}
