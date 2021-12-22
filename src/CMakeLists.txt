file(GLOB_RECURSE CPP_CLIENT_SRC
                  RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
                  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
                  "${CMAKE_CURRENT_SOURCE_DIR}/**/*.cpp")

set_source_files_properties(${PROTOBUF_GENERATES} PROPERTIES GENERATED TRUE)
add_library(etcd-cpp-api ${CPP_CLIENT_SRC} ${PROTOBUF_GENERATES})
add_dependencies(etcd-cpp-api protobuf_generates)

target_link_libraries(etcd-cpp-api PUBLIC
                      ${Boost_LIBRARIES}
                      ${CPPREST_LIB}
                      ${PROTOBUF_LIBRARIES}
                      ${OPENSSL_LIBRARIES}
                      ${GRPC_LIBRARIES})

target_include_directories(etcd-cpp-api PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../proto/gen)
target_include_directories(etcd-cpp-api PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../proto/gen/proto)

if("${CMAKE_VERSION}" VERSION_LESS "3.14")
    install(TARGETS etcd-cpp-api
            EXPORT etcd-targets
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
else()
    install(TARGETS etcd-cpp-api
            EXPORT etcd-targets)
endif()

