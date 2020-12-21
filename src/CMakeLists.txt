file(GLOB_RECURSE CPP_CLIENT_SRC
                  RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
                  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
                  "${CMAKE_CURRENT_SOURCE_DIR}/**/*.cpp")

set_source_files_properties(${PROTOBUF_GENERATES} PROPERTIES GENERATED TRUE)
add_library(etcd-cpp-api ${CPP_CLIENT_SRC} ${PROTOBUF_GENERATES})
add_dependencies(etcd-cpp-api protobuf_generates)
set_property(TARGET etcd-cpp-api PROPERTY CXX_STANDARD 11)

if(cpprestsdk_FOUND)
    target_link_libraries(etcd-cpp-api PUBLIC cpprestsdk::cpprest)
else()
    target_link_libraries(etcd-cpp-api PUBLIC ${CPPREST_LIB})
endif()

target_link_libraries(etcd-cpp-api PUBLIC
                      ${Boost_LIBRARIES}
                      ${PROTOBUF_LIBRARIES}
                      ${OPENSSL_LIBRARIES}
                      ${GRPC_LIBRARIES})

target_include_directories(etcd-cpp-api PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../proto/gen)
target_include_directories(etcd-cpp-api PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../proto/gen/proto)

if (WIN32 AND BUILD_SHARED_LIBS)
    install (TARGETS etcd-cpp-api RUNTIME DESTINATION bin)
else()
    install (TARGETS etcd-cpp-api LIBRARY DESTINATION lib)
endif()
