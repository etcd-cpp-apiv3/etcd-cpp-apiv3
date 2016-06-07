add_library(etcd-cpp-api SHARED ../proto/kv.pb.cc ../proto/auth.pb.cc ../proto/rpc.pb.cc ../proto/rpc.grpc.pb.cc Client.cpp Response.cpp Value.cpp json_constants.cpp DeleteRpcResponse.cpp)
set_property(TARGET etcd-cpp-api PROPERTY CXX_STANDARD 11)

target_link_libraries(etcd-cpp-api ${CPPREST_LIB} boost_system ssl crypto protobuf grpc++)

install (TARGETS etcd-cpp-api DESTINATION lib)
install (FILES ../etcd/Client.hpp
               ../etcd/Response.hpp
               ../etcd/Value.hpp
               DESTINATION include/etcd)
