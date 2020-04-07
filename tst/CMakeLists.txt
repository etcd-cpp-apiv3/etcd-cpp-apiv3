find_path(CATCH_INCLUDE_DIR NAMES catch.hpp PATHS ${PROJECT_SOURCE_DIR})
include_directories(${CATCH_INCLUDE_DIR})

add_executable(etcd_test EtcdTest.cpp
                         EtcdSyncTest.cpp
                         WatcherTest.cpp
                         LockTest.cpp)
set_property(TARGET etcd_test PROPERTY CXX_STANDARD 11)
target_include_directories(etcd_test PRIVATE ${CMAKE_SOURCE_DIR}/proto)

target_link_libraries(etcd_test etcd-cpp-api)

add_test(etcd_test etcd_test)
