cmake_minimum_required (VERSION 3.1.3 FATAL_ERROR)
project (etcd-cpp-api)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set (etcd-cpp-api_VERSION_MAJOR 0)
set (etcd-cpp-api_VERSION_MINOR 1)

find_library(CPPREST_LIB NAMES cpprest)
find_path(CPPREST_INCLUDE_DIR NAMES cpprest/http_client.h)

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

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindGRPC.cmake)
set(GRPC_LIBRARIES ${GPR_LIBRARY} ${GRPC_LIBRARY} ${GRPC_GRPC++_LIBRARY})

# will set `PROTOBUF_GENERATES`, indicates all generated .cc files, and a target `protobuf_generates`.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/proto)

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
