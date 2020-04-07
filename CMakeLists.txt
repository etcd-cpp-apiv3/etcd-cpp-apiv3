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

set(GRPC_LIBRARY_PATH /usr/lib
                      /usr/lib64
                      /usr/local/lib
                      /usr/local/lib64
                      /usr/local/opt/grpc)
find_library(GPR_LIBRARY NAMES gpr PATHS ${GRPC_LIBRARY_PATH})
find_library(GRPC_LIBRARY NAMES grpc PATHS ${GRPC_LIBRARY_PATH})
find_library(GRPC++_LIBRARY NAMES grpc++ PATHS ${GRPC_LIBRARY_PATH})
set(GRPC_LIBRARIES ${GPR_LIBRARY} ${GRPC_LIBRARY} ${GRPC++_LIBRARY})

file(GLOB_RECURSE PROTO_SRC RELATIVE "${CMAKE_SOURCE_DIR}/proto" "proto/*.proto")

add_custom_target(proto-gen
        COMMAND protoc -I . --cpp_out=. ${PROTO_SRC}
        COMMAND protoc -I . --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ./rpc.proto ./v3lock.proto
        COMMENT "Generate protobuf stuffs"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/proto)

enable_testing()
include_directories(SYSTEM ${CPPREST_INCLUDE_DIR}
                           ${Boost_INCLUDE_DIR}
                           ${PROTOBUF_INCLUDE_DIRS}
                           ${OPENSSL_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Werror -Wno-string-compare -std=c++11")

add_subdirectory(src)
add_subdirectory(tst)
