find_path(CATCH_INCLUDE_DIR NAMES catch.hpp PATHS ${PROJECT_SOURCE_DIR})
include_directories(${CATCH_INCLUDE_DIR})

add_custom_target(check ${CMAKE_COMMAND} -E env CTEST_OUTPUT_ON_FAILURE=1
                        ${CMAKE_CTEST_COMMAND} -C $<CONFIG> --verbose
                        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

add_custom_target(etcd_tests)
add_dependencies(check etcd_tests)

file(GLOB TEST_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
foreach(testfile ${TEST_FILES})
    string(REGEX MATCH "^(.*)\\.[^.]*$" dummy ${testfile})
    set(test_name ${CMAKE_MATCH_1})
    message(STATUS "Found unit_test - " ${test_name})
    if(BUILD_ETCD_TESTS)
        add_executable(${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/${testfile})
    else()
        add_executable(${test_name} EXCLUDE_FROM_ALL ${CMAKE_CURRENT_SOURCE_DIR}/${testfile})
    endif()
    use_cxx(${test_name})
    set_exceptions(${test_name})
    add_test(NAME ${test_name} COMMAND $<TARGET_FILE:${test_name}>)

    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../proto/gen)
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../proto/gen/proto)
    target_link_libraries(${test_name} PRIVATE etcd-cpp-api)

    if(UNIX AND NOT APPLE)
        if(CMAKE_VERSION VERSION_LESS 3.13)
            target_link_libraries(${test_name} PRIVATE -Wl,--no-as-needed -lSegFault -Wl,--as-needed)
        else()
            target_link_options(${test_name} PRIVATE -Wl,--no-as-needed -lSegFault -Wl,--as-needed)
        endif()
    endif()

    add_dependencies(etcd_tests ${test_name})
endforeach()
