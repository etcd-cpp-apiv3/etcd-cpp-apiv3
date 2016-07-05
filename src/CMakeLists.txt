add_library(etcd-cpp-api SHARED ../proto/kv.pb.cc ../proto/auth.pb.cc ../proto/rpc.pb.cc ../proto/rpc.grpc.pb.cc ../v3/src/AsyncTxnResponse.cpp ../v3/src/AsyncRangeResponse.cpp ../v3/src/Transaction.cpp ../v3/src/action_constants.cpp ../v3/src/AsyncSetAction.cpp ../v3/src/AsyncCompareAndSwapAction.cpp ../v3/src/AsyncUpdateAction.cpp ../v3/src/AsyncGetAction.cpp ../v3/src/AsyncDeleteAction.cpp ../v3/src/AsyncCompareAndDeleteAction.cpp ../v3/src/Action.cpp ../v3/src/AsyncWatchAction.cpp Client.cpp Response.cpp Value.cpp SyncClient.cpp Watcher.cpp ../v3/src/AsyncWatchResponse.cpp)
set_property(TARGET etcd-cpp-api PROPERTY CXX_STANDARD 11)

target_link_libraries(etcd-cpp-api ${CPPREST_LIB} boost_system ssl crypto protobuf grpc++)

install (TARGETS etcd-cpp-api DESTINATION lib)
install (FILES ../etcd/Client.hpp
               ../etcd/Response.hpp
               ../etcd/Value.hpp
               DESTINATION include/etcd)

