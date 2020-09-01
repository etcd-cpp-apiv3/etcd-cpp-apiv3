function(protobuf_generate_latest)
  set(_options APPEND_PATH DESCRIPTORS)
  set(_singleargs LANGUAGE OUT_VAR EXPORT_MACRO PROTOC_OUT_DIR)
  if(COMMAND target_sources)
    list(APPEND _singleargs TARGET)
  endif()
  set(_multiargs PROTOS IMPORT_DIRS GENERATE_EXTENSIONS)

  cmake_parse_arguments(protobuf_generate_latest "${_options}" "${_singleargs}" "${_multiargs}" "${ARGN}")

  if(NOT protobuf_generate_latest_PROTOS AND NOT protobuf_generate_latest_TARGET)
    message(SEND_ERROR "Error: protobuf_generate_latest called without any targets or source files")
    return()
  endif()

  if(NOT protobuf_generate_latest_OUT_VAR AND NOT protobuf_generate_latest_TARGET)
    message(SEND_ERROR "Error: protobuf_generate_latest called without a target or output variable")
    return()
  endif()

  if(NOT protobuf_generate_latest_LANGUAGE)
    set(protobuf_generate_latest_LANGUAGE cpp)
  endif()
  string(TOLOWER ${protobuf_generate_latest_LANGUAGE} protobuf_generate_latest_LANGUAGE)

  if(NOT protobuf_generate_latest_PROTOC_OUT_DIR)
    set(protobuf_generate_latest_PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
  endif()

  if(protobuf_generate_latest_EXPORT_MACRO AND protobuf_generate_latest_LANGUAGE STREQUAL cpp)
    set(_dll_export_decl "dllexport_decl=${protobuf_generate_latest_EXPORT_MACRO}:")
  endif()

  if(NOT protobuf_generate_latest_GENERATE_EXTENSIONS)
    if(protobuf_generate_latest_LANGUAGE STREQUAL cpp)
      set(protobuf_generate_latest_GENERATE_EXTENSIONS .pb.h .pb.cc)
    elseif(protobuf_generate_latest_LANGUAGE STREQUAL python)
      set(protobuf_generate_latest_GENERATE_EXTENSIONS _pb2.py)
    else()
      message(SEND_ERROR "Error: protobuf_generate_latest given unknown Language ${LANGUAGE}, please provide a value for GENERATE_EXTENSIONS")
      return()
    endif()
  endif()

  if(protobuf_generate_latest_TARGET)
    get_target_property(_source_list ${protobuf_generate_latest_TARGET} SOURCES)
    foreach(_file ${_source_list})
      if(_file MATCHES "proto$")
        list(APPEND protobuf_generate_latest_PROTOS ${_file})
      endif()
    endforeach()
  endif()

  if(NOT protobuf_generate_latest_PROTOS)
    message(SEND_ERROR "Error: protobuf_generate_latest could not find any .proto files")
    return()
  endif()

  if(protobuf_generate_latest_APPEND_PATH)
    # Create an include path for each file specified
    foreach(_file ${protobuf_generate_latest_PROTOS})
      get_filename_component(_abs_file ${_file} ABSOLUTE)
      get_filename_component(_abs_path ${_abs_file} PATH)
      list(FIND _protobuf_include_path ${_abs_path} _contains_already)
      if(${_contains_already} EQUAL -1)
          list(APPEND _protobuf_include_path -I ${_abs_path})
      endif()
    endforeach()
  else()
    set(_protobuf_include_path -I ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  foreach(DIR ${protobuf_generate_latest_IMPORT_DIRS})
    get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
    list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
    if(${_contains_already} EQUAL -1)
        list(APPEND _protobuf_include_path -I ${ABS_PATH})
    endif()
  endforeach()

  set(_generated_srcs_all)
  foreach(_proto ${protobuf_generate_latest_PROTOS})
    get_filename_component(_abs_file ${_proto} ABSOLUTE)
    get_filename_component(_abs_dir ${_abs_file} DIRECTORY)
    get_filename_component(_basename ${_proto} NAME_WE)
    file(RELATIVE_PATH _rel_dir ${CMAKE_CURRENT_SOURCE_DIR} ${_abs_dir})

    set(_possible_rel_dir)
    if (NOT protobuf_generate_latest_APPEND_PATH)
        set(_possible_rel_dir ${_rel_dir}/)
    endif()

    set(_generated_srcs)
    foreach(_ext ${protobuf_generate_latest_GENERATE_EXTENSIONS})
      list(APPEND _generated_srcs "${protobuf_generate_latest_PROTOC_OUT_DIR}/${_possible_rel_dir}${_basename}${_ext}")
    endforeach()

    if(protobuf_generate_latest_DESCRIPTORS AND protobuf_generate_latest_LANGUAGE STREQUAL cpp)
      set(_descriptor_file "${CMAKE_CURRENT_BINARY_DIR}/${_basename}.desc")
      set(_dll_desc_out "--descriptor_set_out=${_descriptor_file}")
      list(APPEND _generated_srcs ${_descriptor_file})
    endif()
    list(APPEND _generated_srcs_all ${_generated_srcs})

    add_custom_command(
      OUTPUT ${_generated_srcs}
      COMMAND  protobuf::protoc
      ARGS --${protobuf_generate_latest_LANGUAGE}_out ${_dll_export_decl}${protobuf_generate_latest_PROTOC_OUT_DIR} ${_dll_desc_out} ${_protobuf_include_path} ${_abs_file}
      DEPENDS ${_abs_file} protobuf::protoc
      COMMENT "Running ${protobuf_generate_latest_LANGUAGE} protocol buffer compiler on ${_proto}"
      VERBATIM )
  endforeach()

  set_source_files_properties(${_generated_srcs_all} PROPERTIES GENERATED TRUE)
  if(protobuf_generate_latest_OUT_VAR)
    set(${protobuf_generate_latest_OUT_VAR} ${_generated_srcs_all} PARENT_SCOPE)
  endif()
  if(protobuf_generate_latest_TARGET)
    target_sources(${protobuf_generate_latest_TARGET} PRIVATE ${_generated_srcs_all})
  endif()
endfunction()

# Compute generated .cc files, workaround for bugs in FindProtobuf.cmake of cmake-3.14.
function(compute_generated_srcs OUT_VAR GENERATED_OUTPUT_DIR GRPC)
    set(${OUT_VAR})
    foreach(_proto_file ${ARGN})
        get_filename_component(_abs_file ${_proto_file} ABSOLUTE)
        get_filename_component(_abs_dir ${_abs_file} DIRECTORY)
        get_filename_component(_basename ${_proto_file} NAME_WE)
        file(RELATIVE_PATH _rel_dir ${CMAKE_CURRENT_SOURCE_DIR} ${_abs_dir})

        set(_ext ".pb.cc")
        if(GRPC)
            set(_ext ".grpc.pb.cc")
        endif()
        list(APPEND ${OUT_VAR} "${GENERATED_OUTPUT_DIR}/${_rel_dir}/${_basename}${_ext}")
    endforeach()
    set(${OUT_VAR} ${${OUT_VAR}} PARENT_SCOPE)
endfunction()
