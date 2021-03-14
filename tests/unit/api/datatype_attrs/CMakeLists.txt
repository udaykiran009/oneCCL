set (executable "datatype_attr_suite")

enable_testing()

if(MULTI_GPU_SUPPORT)
    set (DEFINITIONS "-DCOMM_SUPPORT")
endif()

set(PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}")

include(${PROJECT_SOURCE_DIR}/cmake/helpers.cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
if (COMPUTE_BACKEND)
    activate_compute_backend("${PROJECT_SOURCE_DIR}/cmake" ${COMPUTE_BACKEND})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPUTE_BACKEND_FLAGS}")
    add_definitions(${DEFINITIONS})
endif()

include_directories(${PROJECT_SOURCE_DIR}/tests/googletest-release-1.8.1/googletest/include
                    ${PROJECT_SOURCE_DIR}/tests/googletest-release-1.8.1/googletest/src
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${PROJECT_SOURCE_DIR}/include
                    ${PROJECT_SOURCE_DIR}/src
                    ${PROJECT_SOURCE_DIR}/src/atl)

file(GLOB sources "*.c" "*.cpp")

set (dependencies_sources
             "${PROJECT_SOURCE_DIR}/src/ccl_app_api_datatype_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/utils/version.cpp")

foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} ${src} ${dependencies_sources})
    add_dependencies(${executable} gtest)
    #if(API_UT)
        if (COMPUTE_BACKEND)
            target_include_directories(${executable} PRIVATE
                                                    $<TARGET_PROPERTY:${COMPUTE_BACKEND_TARGET_NAME},INTERFACE_INCLUDE_DIRECTORIES>)
            set_target_properties(${executable} PROPERTIES
                                                    INTERFACE_INCLUDE_DIRECTORIES
                                                    $<TARGET_PROPERTY:${COMPUTE_BACKEND_TARGET_NAME},INTERFACE_INCLUDE_DIRECTORIES>)

            target_link_libraries(${executable} PUBLIC ${COMPUTE_BACKEND_TARGET_NAME})
        endif(COMPUTE_BACKEND)
    #endif(API_UT)
    target_link_libraries(${executable} PUBLIC rt)
    target_link_libraries(${executable} PUBLIC m)
    target_link_libraries(${executable} PUBLIC dl)
    target_link_libraries(${executable} PUBLIC pthread)
    target_link_libraries(${executable} PUBLIC stdc++)
    target_link_libraries(${executable} PUBLIC gtest_main)
    target_link_libraries(${executable} PUBLIC gtest)
endforeach()
