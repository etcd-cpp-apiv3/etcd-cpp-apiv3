cmake_minimum_required (VERSION 3.1.3 FATAL_ERROR)
project (etcd-cpp-api)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(etcd-cpp-api_VERSION_MAJOR 0)
set(etcd-cpp-api_VERSION_MINOR 1)

option(BUILD_ETCD_TESTS "Build test cases" OFF)

find_package(Boost REQUIRED COMPONENTS system thread locale random)
if (APPLE)
    # If we're on OS X check for Homebrew's copy of OpenSSL instead of Apple's
    if (NOT OpenSSL_DIR)
        find_program(HOMEBREW brew)
        if (HOMEBREW STREQUAL "HOMEBREW-NOTFOUND")
            message(WARNING "Homebrew not found: not using Homebrew's OpenSSL")
            if (NOT OPENSSL_ROOT_DIR)
                message(WARNING "Use -DOPENSSL_ROOT_DIR for non-Apple OpenSSL")
            endif()
        else()
            execute_process(COMMAND brew --prefix openssl
                OUTPUT_VARIABLE OPENSSL_ROOT_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE)
        endif()
    endif()
endif()

find_package(OpenSSL REQUIRED)
find_package(Protobuf REQUIRED)
find_package(cpprestsdk REQUIRED)

find_package(gRPC)
if(gRPC_FOUND)
    set(GRPC_LIBRARIES gRPC::gpr gRPC::grpc gRPC::grpc++)
    get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)
else()
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindGRPC.cmake)
    set(GRPC_LIBRARIES ${GPR_LIBRARY} ${GRPC_LIBRARY} ${GRPC_GRPC++_LIBRARY})
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateProtobufGRPC.cmake)

# will set `PROTOBUF_GENERATES`, indicates all generated .cc files, and a target `protobuf_generates`.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateProtobuf.cmake)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/proto)

include_directories(SYSTEM ${Boost_INCLUDE_DIR}
                           ${PROTOBUF_INCLUDE_DIRS}
                           ${OPENSSL_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Werror -Wno-string-compare -std=c++11")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

add_subdirectory(src)

if (BUILD_ETCD_TESTS)
    enable_testing()
    add_subdirectory(tst)
endif ()

install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/etcd/Client.hpp
               ${CMAKE_CURRENT_SOURCE_DIR}/etcd/SyncClient.hpp
               ${CMAKE_CURRENT_SOURCE_DIR}/etcd/Response.hpp
               ${CMAKE_CURRENT_SOURCE_DIR}/etcd/Value.hpp
               ${CMAKE_CURRENT_SOURCE_DIR}/etcd/Watcher.hpp
               DESTINATION include/etcd)
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/etcd/v3/Transaction.hpp
               DESTINATION include/etcd/v3)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/proto/gen/proto/auth.pb.h
               ${CMAKE_CURRENT_BINARY_DIR}/proto/gen/proto/kv.pb.h
               ${CMAKE_CURRENT_BINARY_DIR}/proto/gen/proto/rpc.pb.h
               ${CMAKE_CURRENT_BINARY_DIR}/proto/gen/proto/rpc.grpc.pb.h
               ${CMAKE_CURRENT_BINARY_DIR}/proto/gen/proto/v3lock.pb.h
               ${CMAKE_CURRENT_BINARY_DIR}/proto/gen/proto/v3lock.grpc.pb.h
               DESTINATION include/etcd/proto)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/proto/gen/proto/gogoproto/gogo.pb.h
               DESTINATION include/etcd/proto/gogoproto)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/proto/gen/proto/google/api/annotations.pb.h
               ${CMAKE_CURRENT_BINARY_DIR}/proto/gen/proto/google/api/http.pb.h
               DESTINATION include/etcd/proto/google/api)
