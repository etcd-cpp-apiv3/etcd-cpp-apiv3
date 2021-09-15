file(GLOB_RECURSE PROTO_SRCS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.proto")

set(PROTO_GEN_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen/proto")
file(MAKE_DIRECTORY "${PROTO_GEN_OUT_DIR}")

# use `protobuf_generate` rather than `protobuf_generate_cpp` since we want to
# output the generated files to source dir, rather than binary dir.
protobuf_generate_latest(
    LANGUAGE cpp
    OUT_VAR PROTO_GENERATES
    PROTOC_OUT_DIR "${PROTO_GEN_OUT_DIR}"
    PROTOS ${PROTO_SRCS}
)
compute_generated_srcs(PROTO_GENERATES_SRCS "${PROTO_GEN_OUT_DIR}" false ${PROTO_SRCS})

set(PROTO_GRPC_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto"
                    "${CMAKE_CURRENT_SOURCE_DIR}/v3election.proto"
                    "${CMAKE_CURRENT_SOURCE_DIR}/v3lock.proto")
grpc_generate_cpp(PROTO_GRPC_GENERATES PROTO_GRPC_GENERATES_HDRS
    "${PROTO_GEN_OUT_DIR}"
    ${PROTO_GRPC_SRCS}
)
compute_generated_srcs(PROTO_GRPC_GENERATES_SRCS "${PROTO_GEN_OUT_DIR}" true ${PROTO_GRPC_SRCS})

# populate `PROTOBUF_GENERATES` in the parent scope.
set(PROTOBUF_GENERATE_DEPS)
foreach(cxx_file ${PROTO_GENERATES})
    if(cxx_file MATCHES "cc$")
        list(APPEND PROTOBUF_GENERATE_DEPS ${cxx_file})
    endif()
endforeach()
foreach(cxx_file ${PROTO_GRPC_GENERATES})
    list(APPEND PROTOBUF_GENERATE_DEPS ${cxx_file})
endforeach()

set(PROTOBUF_GENERATES ${PROTO_GENERATES_SRCS} ${PROTO_GRPC_GENERATES_SRCS})
set(PROTOBUF_GENERATES ${PROTOBUF_GENERATES} PARENT_SCOPE)
set_source_files_properties(${PROTOBUF_GENERATES} PROPERTIES GENERATED TRUE)
add_custom_target(protobuf_generates DEPENDS ${PROTOBUF_GENERATE_DEPS})
