set (executable "stream_suite")


enable_testing()
message ("PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")

if(API_UT)
    set(PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/../..")
endif()

include(${PROJECT_SOURCE_DIR}/cmake/helpers.cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

#add_subdirectory (${PROJECT_SOURCE_DIR}/tests/functional/googletest-release-1.8.1/googletest/ ${PROJECT_SOURCE_DIR}/tests/units)
include_directories(${PROJECT_SOURCE_DIR}/tests/functional/googletest-release-1.8.1/googletest/include
                    ${PROJECT_SOURCE_DIR}/tests/functional/googletest-release-1.8.1/googletest/src
                    ${CMAKE_CURRENT_BINARY_DIR}/..
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${PROJECT_SOURCE_DIR}/include
                    ${PROJECT_SOURCE_DIR}/src
                    ${PROJECT_SOURCE_DIR}/src/atl)

file(GLOB sources "*.c" "*.cpp")

set (dependencies_sources
             "${PROJECT_SOURCE_DIR}/src/ccl_cpp_utils.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/global.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/env.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/cache.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/datatype.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/native_runtime_utils.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/atl_wrapper.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/ccl_unordered_coll_manager.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/utils/version.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/ccl_allreduce_2d_builder.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/log/log.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/stream/stream.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_cpp_stream.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/spinlock.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/utils/version.cpp")

foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} $<TARGET_OBJECTS:native_cl_api_objects> ${src} ${dependencies_sources})
    add_dependencies(${executable} gtest)
    if (COMPUTE_BACKEND)
        target_include_directories(${executable} PRIVATE
                                                $<TARGET_PROPERTY:${COMPUTE_BACKEND_TARGET_NAME},INTERFACE_INCLUDE_DIRECTORIES>)
        set_target_properties(${executable} PROPERTIES
                                                INTERFACE_INCLUDE_DIRECTORIES
                                                $<TARGET_PROPERTY:${COMPUTE_BACKEND_TARGET_NAME},INTERFACE_INCLUDE_DIRECTORIES>)

        target_link_libraries(${executable} PUBLIC ${COMPUTE_BACKEND_TARGET_NAME})
    endif(COMPUTE_BACKEND)

    target_link_libraries(${executable} PUBLIC rt)
    target_link_libraries(${executable} PUBLIC m)
    target_link_libraries(${executable} PUBLIC dl)
    target_link_libraries(${executable} PUBLIC pthread)
    target_link_libraries(${executable} PUBLIC stdc++)
    target_link_libraries(${executable} PUBLIC gtest_main)
    target_link_libraries(${executable} PUBLIC gtest)
endforeach()
