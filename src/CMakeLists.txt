# grpc stuffs
set_source_files_properties(${PROTOBUF_GENERATES} PROPERTIES GENERATED TRUE)

macro(include_generated_protobuf_files target)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../proto/gen)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../proto/gen/proto)
endmacro()

# prepare common objects
file(GLOB_RECURSE CPP_CLIENT_CORE_SRC
                  RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
                  "${CMAKE_CURRENT_SOURCE_DIR}/KeepAlive.cpp"
                  "${CMAKE_CURRENT_SOURCE_DIR}/Response.cpp"
                  "${CMAKE_CURRENT_SOURCE_DIR}/SyncClient.cpp"
                  "${CMAKE_CURRENT_SOURCE_DIR}/Value.cpp"
                  "${CMAKE_CURRENT_SOURCE_DIR}/Watcher.cpp"
                  "${CMAKE_CURRENT_SOURCE_DIR}/**/*.cpp"
)

add_library(etcd-cpp-api-core-objects OBJECT ${CPP_CLIENT_CORE_SRC} ${PROTOBUF_GENERATES})
use_cxx(etcd-cpp-api-core-objects)
set_exceptions(etcd-cpp-api-core-objects)
add_dependencies(etcd-cpp-api-core-objects protobuf_generates)
include_generated_protobuf_files(etcd-cpp-api-core-objects)
target_link_libraries(etcd-cpp-api-core-objects PUBLIC
                      ${OPENSSL_LIBRARIES}
                      ${GRPC_LIBRARIES}
)
if(TARGET protobuf::libprotobuf)
    target_link_libraries(etcd-cpp-api-core-objects PUBLIC protobuf::libprotobuf)
else()
    target_link_libraries(etcd-cpp-api-core-objects PUBLIC ${PROTOBUF_LIBRARIES})
endif()

if(BUILD_ETCD_CORE_ONLY)
    # add the core library, includes the sycnhronous client only
    add_library(etcd-cpp-api-core $<TARGET_OBJECTS:etcd-cpp-api-core-objects>)
    use_cxx(etcd-cpp-api-core)
    set_exceptions(etcd-cpp-api-core)
    target_link_libraries(etcd-cpp-api-core PUBLIC
                          ${OPENSSL_LIBRARIES}
                          ${GRPC_LIBRARIES}
    )
    if(TARGET protobuf::libprotobuf)
        target_link_libraries(etcd-cpp-api-core PUBLIC protobuf::libprotobuf)
    else()
        target_link_libraries(etcd-cpp-api-core PUBLIC ${PROTOBUF_LIBRARIES})
    endif()
    include_generated_protobuf_files(etcd-cpp-api-core)
else()
    # add the client with asynchronus client
    add_library(etcd-cpp-api $<TARGET_OBJECTS:etcd-cpp-api-core-objects>
                            "${CMAKE_CURRENT_SOURCE_DIR}/Client.cpp")
    use_cxx(etcd-cpp-api)
    set_exceptions(etcd-cpp-api)
    target_link_libraries(etcd-cpp-api PUBLIC
                          ${CPPREST_LIB}  # n.b.: the asynchronous client requires pplx in cpprestsdk
                          ${OPENSSL_LIBRARIES}
                          ${GRPC_LIBRARIES}
    )
    if(TARGET protobuf::libprotobuf)
        target_link_libraries(etcd-cpp-api PUBLIC protobuf::libprotobuf)
    else()
        target_link_libraries(etcd-cpp-api PUBLIC ${PROTOBUF_LIBRARIES})
    endif()
    include_generated_protobuf_files(etcd-cpp-api)
endif()

if(BUILD_ETCD_CORE_ONLY)
    add_library(etcd-cpp-api INTERFACE)
    target_link_libraries(etcd-cpp-api INTERFACE etcd-cpp-api-core)
else()
    add_library(etcd-cpp-api-core INTERFACE)
    target_link_libraries(etcd-cpp-api-core INTERFACE etcd-cpp-api)
endif()

if("${CMAKE_VERSION}" VERSION_LESS "3.14")
    install(TARGETS etcd-cpp-api-core etcd-cpp-api
            EXPORT etcd-targets
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
else()
    install(TARGETS etcd-cpp-api-core etcd-cpp-api
            EXPORT etcd-targets)
endif()
