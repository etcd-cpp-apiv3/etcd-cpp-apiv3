# - Config file for the etcd-cpp-apiv3 package
#
# It defines the following variables
#
#  ETCD_CPP_INCLUDE_DIR         - include directory for etcd
#  ETCD_CPP_INCLUDE_DIRS        - include directories for etcd
#  ETCD_CPP_LIBRARIES           - libraries to link against

# find dependencies
include(CMakeFindDependencyMacro)
find_dependency(Protobuf)
find_package(gRPC QUIET)
if(NOT gRPC_FOUND)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
    find_dependency(GRPC)
endif()
find_dependency(cpprestsdk)
if(cpprestsdk_FOUND)
    set(CPPREST_LIB cpprestsdk::cpprest)
endif()

set(ETCD_CPP_HOME "${CMAKE_CURRENT_LIST_DIR}/../../..")
include("${CMAKE_CURRENT_LIST_DIR}/etcd-targets.cmake")

set(etcd-cpp-api_FOUND TRUE)
set(ETCD_CPP_LIBRARIES etcd-cpp-api)
set(ETCD_CPP_INCLUDE_DIR "${ETCD_CPP_HOME}/include")
set(ETCD_CPP_INCLUDE_DIRS "${ETCD_CPP_INCLUDE_DIR}")

include(FindPackageMessage)
find_package_message(etcd
    "Found etcd: ${CMAKE_CURRENT_LIST_FILE} (found version \"@etcd-cpp-api_VERSION@\")"
    "etcd-cpp-apiv3 version: @etcd-cpp-api_VERSION@\netcd-cpp-apiv3 libraries: ${ETCD_CPP_LIBRARIES}, include directories: ${ETCD_CPP_INCLUDE_DIRS}"
)
