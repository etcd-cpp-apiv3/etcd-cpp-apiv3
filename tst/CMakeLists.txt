add_executable(etcd_test EtcdTest.cpp)
set_property(TARGET etcd_test PROPERTY CXX_STANDARD 11)

target_link_libraries(etcd_test etcd-cpp-api)

add_test(etcd_test etcd_test)
