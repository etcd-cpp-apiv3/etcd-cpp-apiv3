cmake_minimum_required (VERSION 3.1.3 FATAL_ERROR)
project (etcd-cpp-api)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(etcd-cpp-api_VERSION_MAJOR 0)
set(etcd-cpp-api_VERSION_MINOR 2)
set(etcd-cpp-api_VERSION_PATCH 3)
set(etcd-cpp-api_VERSION ${etcd-cpp-api_VERSION_MAJOR}.${etcd-cpp-api_VERSION_MINOR}.${etcd-cpp-api_VERSION_PATCH})
set(CMAKE_PROJECT_HOMEPAGE_URL https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3)

include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)
include(CheckLibraryExists)
include(GNUInstallDirs)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(DEFAULT_BUILD_TYPE "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE
        STRING "Choose the type of build." FORCE
    )
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo"
    )
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_ETCD_TESTS "Build test cases" OFF)

# reference: https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling#always-full-rpath
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib:${CMAKE_INSTALL_PREFIX}/lib64")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    set(CMAKE_GNUtoMS ON)
endif()

find_package(Boost REQUIRED COMPONENTS system thread random)
if(APPLE)
    # If we're on OS X check for Homebrew's copy of OpenSSL instead of Apple's
    if(NOT OpenSSL_DIR)
        find_program(HOMEBREW brew)
        if(HOMEBREW STREQUAL "HOMEBREW-NOTFOUND")
            message(WARNING "Homebrew not found: not using Homebrew's OpenSSL")
            if(NOT OPENSSL_ROOT_DIR)
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
if(Protobuf_PROTOC_EXECUTABLE)
    if(NOT TARGET protobuf::protoc)
        add_executable(protobuf::protoc IMPORTED)
        if(EXISTS "${Protobuf_PROTOC_EXECUTABLE}")
            set_target_properties(protobuf::protoc PROPERTIES
                IMPORTED_LOCATION "${Protobuf_PROTOC_EXECUTABLE}")
        endif()
    endif()
endif()
find_package(cpprestsdk QUIET)
if(cpprestsdk_FOUND)
    set(CPPREST_INCLUDE_DIR)
    set(CPPREST_LIB cpprestsdk::cpprest)
else()
    find_library(CPPREST_LIB NAMES cpprest)
    find_path(CPPREST_INCLUDE_DIR NAMES cpprest/http_client.h)
endif()

find_package(gRPC QUIET)
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
                           ${CPPREST_INCLUDE_DIR}
                           ${PROTOBUF_INCLUDE_DIRS}
                           ${OPENSSL_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Werror -Wno-string-compare")
endif()

set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES} ${GRPC_INCLUDE_DIR}")
set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES} ${GRPC_LIBRARIES}")
set(CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS}")
check_cxx_source_compiles("
#include <grpc++/grpc++.h>
namespace grpc { class Channel; }
int main() {}
" GRPC_CHANNEL_CLASS_FOUND)
if(GRPC_CHANNEL_CLASS_FOUND)
    add_definitions(-DWITH_GRPC_CHANNEL_CLASS)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

enable_testing()
add_subdirectory(src)
add_subdirectory(tst)

install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/etcd/Client.hpp
               ${CMAKE_CURRENT_SOURCE_DIR}/etcd/KeepAlive.hpp
               ${CMAKE_CURRENT_SOURCE_DIR}/etcd/SyncClient.hpp
               ${CMAKE_CURRENT_SOURCE_DIR}/etcd/Response.hpp
               ${CMAKE_CURRENT_SOURCE_DIR}/etcd/Value.hpp
               ${CMAKE_CURRENT_SOURCE_DIR}/etcd/Watcher.hpp
               DESTINATION include/etcd)
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/etcd/v3/action_constants.hpp
               ${CMAKE_CURRENT_SOURCE_DIR}/etcd/v3/Transaction.hpp
               DESTINATION include/etcd/v3)

configure_file(etcd-cpp-api-config.in.cmake
               "${PROJECT_BINARY_DIR}/etcd-cpp-api-config.cmake" @ONLY
)
configure_file(etcd-cpp-api-config-version.in.cmake
               "${PROJECT_BINARY_DIR}/etcd-cpp-api-config-version.cmake" @ONLY
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindGRPC.cmake"
              "${PROJECT_BINARY_DIR}/etcd-cpp-api-config.cmake"
              "${PROJECT_BINARY_DIR}/etcd-cpp-api-config-version.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/etcd-cpp-api
)
install(EXPORT etcd-targets
        FILE etcd-targets.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/etcd-cpp-api
)

# deb/rpc packaging for Linux

set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR etcd-cpp-apiv3)
set(CPACK_PACKAGE_CONTACT "Tao He <sighingnow@gmail.com>")
set(CPACK_PACKAGE_VERSION "${etcd-cpp-api_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${etcd-cpp-api_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${etcd-cpp-api_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${etcd-cpp-api_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The etcd-cpp-apiv3 is a C++ API for etcd's v3 client API, i.e., `ETCDCTL_API=3`.")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_SOURCE_IGNORE_FILES ".git" "${PROJECT_BINARY_DIR}")

set(CPACK_CMAKE_GENERATOR "Unix Makefiles")
set(CPACK_GENERATOR "STGZ;TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")
include(CPack)

# generate deb for ppa
#
# requires: apt-get install devscripts
set(CPACK_DEBIAN_BUILD_DEPENDS   "ca-certificates,
                                  debhelper,
                                  gcc,
                                  g++,
                                  libboost-all-dev,
                                  libcpprest-dev,
                                  libcurl4-openssl-dev,
                                  libgrpc-dev,
                                  libgrpc++-dev,
                                  libprotobuf-dev,
                                  libprotoc-dev,
                                  libssl-dev,
                                  libz-dev,
                                  lsb-release,
                                  openssl,
                                  protobuf-compiler-grpc")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcpprest-dev,
                                  libprotobuf-dev,
                                  libgrpc-dev,
                                  libgrpc++-dev,
                                  libssl-dev")

set(CPACK_DEBIAN_PACKAGE_UPSTREAM_COPYRIGHT_YEAR 2016-2021)
set(CPACK_DEBIAN_PACKAGE_LICENSE bsd)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Tao He <sighingnow@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3/")

set(CPACK_DEBIAN_PACKAGE_INSTALL "/usr/lib/lib*.so*"
                                 "/usr/lib/cmake/etcd-cpp-api/*.cmake"
                                 "/usr/include/etcd/*.h"
                                 "/usr/include/etcd/*.hpp"
                                 "/usr/include/etcd/v3/action_constants.hpp"
                                 "/usr/include/etcd/v3/Transaction.hpp"
)

set(CPACK_DEBIAN_PACKAGE_BUILD_NUMBER_PREFIX "")
set(CPACK_DEBIAN_PACKAGE_BUILD_NUMBER 0)

set(CPACK_DEBIAN_PACKAGE_DISTRIBUTION "focal")
set(DPUT_HOST "ppa:graphscope/etcd-cpp-api")
set(DPUT_SNAPSHOT_HOST "ppa:graphscope/etcd-cpp-api")

find_program(DEBUILD_EXECUTABLE debuild)
find_program(DPUT_EXECUTABLE dput)

if(DEBUILD_EXECUTABLE AND DPUT_EXECUTABLE)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    include(UploadPPA)
endif()
