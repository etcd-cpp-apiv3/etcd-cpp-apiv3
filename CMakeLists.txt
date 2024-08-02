cmake_minimum_required (VERSION 3.3 FATAL_ERROR)
project (etcd-cpp-api)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(etcd-cpp-api_VERSION_MAJOR 0)
set(etcd-cpp-api_VERSION_MINOR 15)
set(etcd-cpp-api_VERSION_PATCH 4)
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

option(BUILD_WITH_NO_EXCEPTIONS "Build etcd-cpp-apiv3 with disabling exception handling, i.e., -fno-exceptions" OFF)
option(BUILD_SHARED_LIBS "Build etcd-cpp-apiv3 shared libraries" ON)
option(BUILD_ETCD_CORE_ONLY "Build etcd-cpp-apiv3 core library (the synchronous runtime) only" OFF)
option(BUILD_ETCD_TESTS "Build etcd-cpp-apiv3 test cases" OFF)
option(CMAKE_POSITION_INDEPENDENT_CODE "Build etcd-cpp-apiv3 with -fPIC" ON)
option(ETCD_W_STRICT "Build etcd-cpp-apiv3 with -Werror" ON)
option(ETCD_CMAKE_CXX_STANDARD "Build etcd-cpp-apiv3 with specified C++ standard, e.g., 11, 14, 17, 20" "")

if(NOT "${ETCD_CMAKE_CXX_STANDARD}")
    if(NOT "${CMAKE_CXX_STANDARD}")
        set(ETCD_CMAKE_CXX_STANDARD 11)
    else()
        set(ETCD_CMAKE_CXX_STANDARD ${CMAKE_CXX_STANDARD})
    endif()
endif()
message(STATUS "Building etcd-cpp-apiv3 with C++${ETCD_CMAKE_CXX_STANDARD}")

# reference: https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling#always-full-rpath
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib:${CMAKE_INSTALL_PREFIX}/lib64:${CMAKE_INSTALL_PREFIX}/lib/x86_64-linux-gnu")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    set(CMAKE_GNUtoMS ON)
endif()

if(NOT (CMAKE_CXX_COMPILER_LAUNCHER MATCHES "ccache") AND NOT (CMAKE_C_COMPILER_LAUNCHER MATCHES "ccache"))
    find_program(ccache_EXECUTABLE ccache)
    if(ccache_EXECUTABLE)
        set(CMAKE_C_COMPILER_LAUNCHER ${ccache_EXECUTABLE})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${ccache_EXECUTABLE})
        if (NOT TARGET ccache-stats)
            add_custom_target(ccache-stats
                COMMAND ${ccache_EXECUTABLE} --show-stats
            )
        endif()
    else()
        if (NOT TARGET ccache-stats)
            add_custom_target(ccache-stats
                COMMAND echo "ccache not found."
            )
        endif()
    endif(ccache_EXECUTABLE)
endif()

macro(use_cxx target)
    if(CMAKE_VERSION VERSION_LESS "3.1")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target} PRIVATE "-std=gnu++${ETCD_CMAKE_CXX_STANDARD}")
        else()
            target_compile_options(${target} PRIVATE "-std=c++${ETCD_CMAKE_CXX_STANDARD}")
        endif()
    elseif(CMAKE_VERSION VERSION_LESS "3.8")
        set_target_properties(${target} PROPERTIES
            CXX_STANDARD ${ETCD_CMAKE_CXX_STANDARD}
            CXX_STANDARD_REQUIRED ON
        )
    else()
        target_compile_features(${target} PUBLIC cxx_std_${ETCD_CMAKE_CXX_STANDARD})
    endif()
endmacro(use_cxx)

macro(set_exceptions target)
    if(BUILD_WITH_NO_EXCEPTIONS)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target} PRIVATE "-fno-exceptions")
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_compile_options(${target} PRIVATE "-fno-exceptions")
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
            target_compile_options(${target} PRIVATE "-fno-exceptions")
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            target_compile_options(${target} PRIVATE "/EHs-c-")
        endif()
        target_compile_definitions(${target} PUBLIC -D_ETCD_NO_EXCEPTIONS)
    endif()
endmacro(set_exceptions)

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
find_package(Protobuf CONFIG QUIET)
if (NOT Protobuf_FOUND)
    find_package(Protobuf REQUIRED)
endif()
if(Protobuf_PROTOC_EXECUTABLE)
    if(NOT TARGET protobuf::protoc)
        add_executable(protobuf::protoc IMPORTED)
        if(EXISTS "${Protobuf_PROTOC_EXECUTABLE}")
            set_target_properties(protobuf::protoc PROPERTIES
                IMPORTED_LOCATION "${Protobuf_PROTOC_EXECUTABLE}")
        endif()
    endif()
endif()

# When cross compiling we look for the native protoc compiler
# overwrite protobuf::protoc with the proper protoc
if(CMAKE_CROSSCOMPILING)
    find_program(Protobuf_PROTOC_EXECUTABLE REQUIRED NAMES protoc)
    if(NOT TARGET protobuf::protoc)
        add_executable(protobuf::protoc IMPORTED)
    endif()
    set_target_properties(protobuf::protoc PROPERTIES
        IMPORTED_LOCATION "${Protobuf_PROTOC_EXECUTABLE}")
endif()

find_package(gRPC QUIET)
if(gRPC_FOUND AND TARGET gRPC::grpc)
    # When cross compiling we look for the native grpc_cpp_plugin
    if(CMAKE_CROSSCOMPILING)
        find_program(GRPC_CPP_PLUGIN REQUIRED NAMES grpc_cpp_plugin)
        if(NOT TARGET gRPC::grpc_cpp_plugin)
            add_executable(gRPC::grpc_cpp_plugin IMPORTED)
        endif()
        set_target_properties(gRPC::grpc_cpp_plugin PROPERTIES
                IMPORTED_LOCATION "${GRPC_CPP_PLUGIN}")
    elseif(TARGET gRPC::grpc_cpp_plugin)
        get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)
    else()
        message(FATAL_ERROR "Found gRPC but no gRPC CPP plugin defined")
    endif()

    set(GRPC_LIBRARIES gRPC::gpr gRPC::grpc gRPC::grpc++)
    get_target_property(GRPC_INCLUDE_DIR gRPC::grpc INTERFACE_INCLUDE_DIRECTORIES)
else()
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindGRPC.cmake)
    set(GRPC_LIBRARIES ${GPR_LIBRARY} ${GRPC_LIBRARY} ${GRPC_GRPC++_LIBRARY})
endif()
message(STATUS "Found GRPC: ${GRPC_LIBRARIES}, ${GRPC_CPP_PLUGIN} (found version: \"${gRPC_VERSION}\")")
# avoid use the apt-get installed libgrpc-dev (version v1.13) on Ubuntu 18.04
if(gRPC_FOUND AND gRPC_VERSION VERSION_LESS "1.14")
    message(FATAL_ERROR "gRPC '${gRPC_VERSION}' is not supported, please install a newer gRPC library "
                        "by following the example below"
                        "\n"
                        "    https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3/blob/master/.github/workflows/build-test.yml#L50-L75")
endif()
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateProtobufGRPC.cmake)

if(gRPC_VERSION VERSION_LESS "1.21" OR gRPC_VERSION VERSION_GREATER "1.31")
    add_definitions(-DWITH_GRPC_CHANNEL_CLASS)
endif()
if(gRPC_VERSION VERSION_LESS "1.17")
    add_definitions(-DWITH_GRPC_CREATE_CHANNEL_INTERNAL_UNIQUE_POINTER)
endif()

# will set `PROTOBUF_GENERATES`, indicates all generated .cc files, and a target `protobuf_generates`.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateProtobuf.cmake)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/proto)

# test cases requires the async runtime
if(BUILD_ETCD_TESTS)
    message(STATUS "Building etcd-cpp-apiv3 with tests requires the async runtime, "
                   "setting BUILD_ETCD_CORE_ONLY to ON ...")
    set(BUILD_ETCD_CORE_ONLY OFF)
endif()

if (NOT BUILD_ETCD_CORE_ONLY)
    find_package(cpprestsdk QUIET)
    if(cpprestsdk_FOUND)
        set(CPPREST_INCLUDE_DIR)
        set(CPPREST_LIB cpprestsdk::cpprest)
    else()
        find_library(CPPREST_LIB NAMES cpprest)
        find_path(CPPREST_INCLUDE_DIR NAMES cpprest/http_client.h)
    endif()
else()
    set(CPPREST_INCLUDE_DIR)
    set(CPPREST_LIB)
endif()

include_directories(SYSTEM ${CPPREST_INCLUDE_DIR}
                           ${PROTOBUF_INCLUDE_DIRS}
                           ${GRPC_INCLUDE_DIR}
                           ${OPENSSL_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    if(ETCD_W_STRICT)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wno-string-compare")
endif()

check_cxx_compiler_flag(-Wno-c++17-extensions W_NO_CPP17_EXTENSIONS)
check_cxx_compiler_flag(-Wno-extra-semi W_NO_EXTRA_SEMI)
if(W_NO_CPP17_EXTENSIONS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++17-extensions")
endif()
if(W_NO_EXTRA_SEMI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-extra-semi")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

add_subdirectory(src)
if(BUILD_ETCD_TESTS)
  enable_testing()
  add_subdirectory(tst)
endif()

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/etcd/KeepAlive.hpp
              ${CMAKE_CURRENT_SOURCE_DIR}/etcd/SyncClient.hpp
              ${CMAKE_CURRENT_SOURCE_DIR}/etcd/Response.hpp
              ${CMAKE_CURRENT_SOURCE_DIR}/etcd/Value.hpp
              ${CMAKE_CURRENT_SOURCE_DIR}/etcd/Watcher.hpp
              DESTINATION include/etcd)
if(NOT BUILD_ETCD_CORE_ONLY)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/etcd/Client.hpp
                  DESTINATION include/etcd)
endif()
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/etcd/v3/action_constants.hpp
              ${CMAKE_CURRENT_SOURCE_DIR}/etcd/v3/Transaction.hpp
              ${CMAKE_CURRENT_SOURCE_DIR}/etcd/v3/Member.hpp
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

file(GLOB_RECURSE FILES_NEED_FORMAT
        "etcd/*.hpp"
        "src/*.cpp"
        "tst/*.cpp"
)

add_custom_target(etcd_cpp_apiv3_clformat
    COMMAND clang-format --style=file -i ${FILES_NEED_FORMAT}
    COMMENT "Running clang-format, using clang-format-11 from https://github.com/muttleyxd/clang-tools-static-binaries/releases"
    VERBATIM)

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

set(CPACK_DEBIAN_PACKAGE_UPSTREAM_COPYRIGHT_YEAR 2016-2023)
set(CPACK_DEBIAN_PACKAGE_LICENSE bsd)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Tao He <sighingnow@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3/")

set(CPACK_DEBIAN_PACKAGE_INSTALL "/usr/lib/lib*.so*"
                                 "/usr/lib/cmake/etcd-cpp-api/*.cmake"
                                 "/usr/include/etcd/*.h"
                                 "/usr/include/etcd/*.hpp"
                                 "/usr/include/etcd/v3/action_constants.hpp"
                                 "/usr/include/etcd/v3/Transaction.hpp"
                                 "/usr/include/etcd/v3/Member.hpp"
)

set(CPACK_DEBIAN_PACKAGE_BUILD_NUMBER_PREFIX "")
set(CPACK_DEBIAN_PACKAGE_BUILD_NUMBER 0)

set(CPACK_DEBIAN_PACKAGE_DISTRIBUTION "focal")
set(DPUT_HOST "ppa:etcd-cpp-apiv3/etcd-cpp-api")
set(DPUT_SNAPSHOT_HOST "ppa:etcd-cpp-apiv3/etcd-cpp-api")

find_program(DEBUILD_EXECUTABLE debuild)
find_program(DPUT_EXECUTABLE dput)

if(DEBUILD_EXECUTABLE AND DPUT_EXECUTABLE)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    include(UploadPPA)
endif()
