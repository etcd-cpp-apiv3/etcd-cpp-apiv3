cmake_minimum_required (VERSION 3.1.3 FATAL_ERROR)
project (etcd-cpp-api)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set (etcd-cpp-api_VERSION_MAJOR 0)
set (etcd-cpp-api_VERSION_MINOR 1)

find_library(CPPREST_LIB NAMES cpprest)
find_path(CPPREST_INCLUDE_DIR NAMES cpprest/http_client.h)

find_package(Boost REQUIRED COMPONENTS system thread locale random)
find_package(OpenSSL REQUIRED)
find_package(Protobuf REQUIRED)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindGRPC.cmake)
set(GRPC_LIBRARIES ${GPR_LIBRARY} ${GRPC_LIBRARY} ${GRPC_GRPC++_LIBRARY})

file(GLOB_RECURSE PROTO_SRC RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/proto" "proto/*.proto")

# Generate protobuf definitions
execute_process(COMMAND ${Protobuf_PROTOC_EXECUTABLE}
                        -I ${CMAKE_CURRENT_SOURCE_DIR}/proto
                        --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto
                        ${PROTO_SRC})
execute_process(COMMAND ${Protobuf_PROTOC_EXECUTABLE}
                        -I ${CMAKE_CURRENT_SOURCE_DIR}/proto
                        --grpc_out=${CMAKE_CURRENT_SOURCE_DIR}/proto
                        --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
                        ${CMAKE_CURRENT_SOURCE_DIR}/proto/rpc.proto
                        ${CMAKE_CURRENT_SOURCE_DIR}/proto/v3lock.proto)

enable_testing()
include_directories(SYSTEM ${CPPREST_INCLUDE_DIR}
                           ${Boost_INCLUDE_DIR}
                           ${PROTOBUF_INCLUDE_DIRS}
                           ${OPENSSL_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Werror -Wno-string-compare -std=c++11")

add_subdirectory(src)

if (BUILD_TESTS)
    add_subdirectory(tst)
endif ()
