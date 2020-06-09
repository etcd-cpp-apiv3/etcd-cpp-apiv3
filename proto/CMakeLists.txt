file(GLOB_RECURSE PROTO_SRCS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.proto")

# use `protobuf_generate` rather than `protobuf_generate_cpp` since we want to
# output the generated files to source dir, rather than binary dir.
protobuf_generate(
    LANGUAGE cpp
    OUT_VAR PROTO_GENERATES
    PROTOC_OUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    PROTOS ${PROTO_SRCS}
)

grpc_generate_cpp(PROTO_GRPC_GENERATES PROTO_GRPC_GENERATES_HDRS
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto"
    "${CMAKE_CURRENT_SOURCE_DIR}/v3lock.proto"
)

# populate `PROTOBUF_GENERATES` in the parent scope.
set(PROTOBUF_GENERATES)
foreach(cxx_file ${PROTO_GENERATES})
    if(cxx_file MATCHES "cc$")
        list(APPEND PROTOBUF_GENERATES ${cxx_file})
    endif()
endforeach()
foreach(cxx_file ${PROTO_GRPC_GENERATES})
    list(APPEND PROTOBUF_GENERATES ${cxx_file})
endforeach()

set(PROTOBUF_GENERATES ${PROTOBUF_GENERATES} PARENT_SCOPE)
set_source_files_properties(${PROTOBUF_GENERATES} PROPERTIES GENERATED TRUE)
add_custom_target(protobuf_generates DEPENDS ${PROTOBUF_GENERATES})
