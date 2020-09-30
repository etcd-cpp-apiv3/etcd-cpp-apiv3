find_path(CATCH_INCLUDE_DIR NAMES catch.hpp PATHS ${PROJECT_SOURCE_DIR})
include_directories(${CATCH_INCLUDE_DIR})

add_custom_target(check ${CMAKE_COMMAND} -E env CTEST_OUTPUT_ON_FAILURE=1
                        ${CMAKE_CTEST_COMMAND} -C $<CONFIG> --verbose
                        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

file(GLOB TEST_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
foreach(testfile ${TEST_FILES})
    string(REGEX MATCH "^(.*)\\.[^.]*$" dummy ${testfile})
    set(test_name ${CMAKE_MATCH_1})
    message(STATUS "Found unit_test - " ${test_name})
    add_executable(${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/${testfile})
    add_test(NAME ${test_name} COMMAND $<TARGET_FILE:${test_name}>)

    set_property(TARGET ${test_name} PROPERTY CXX_STANDARD 11)
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../proto/gen)
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../proto/gen/proto)
    target_link_libraries(${test_name} etcd-cpp-api)

    add_dependencies(check ${test_name})
endforeach()
