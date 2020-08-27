set (executable "device_communicator_suite")


enable_testing()
message ("PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")

if(MULTI_GPU_SUPPORT)
    set (DEFINITIONS "-DDEVICE_COMM_SUPPORT")
    option(CCL_GPU_DEVICES_AFFINITY_ENABLE "Enable L0" ON)
    if(CCL_GPU_DEVICES_AFFINITY_ENABLE)
        set(CCL_GPU_DEVICES_AFFINITY_MASK_SIZE 4)
        message ("Set L0 device mask affinity size: ${CCL_GPU_DEVICES_AFFINITY_MASK_SIZE}")
    endif()
endif()

if(API_UT)
    set(PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/../..")
    include(${PROJECT_SOURCE_DIR}/cmake/helpers.cmake)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    if(COMPUTE_RUNTIME)
        activate_compute_runtime("${PROJECT_SOURCE_DIR}/cmake" ${COMPUTE_RUNTIME})

        if (${CCL_ENABLE_SYCL_V} STREQUAL 1)
            option (CCL_ENABLE_SYCL "Enable CCL SYCL runtime" ON)
            message(STATUS "Enable CCL SYCL runtime")
        endif()

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPUTE_RUNTIME_FLAGS}")
        add_definitions(${DEFINITIONS})
    endif()
endif(API_UT)


include_directories(${PROJECT_SOURCE_DIR}/tests/functional/googletest-release-1.8.1/googletest/include
                    ${PROJECT_SOURCE_DIR}/tests/functional/googletest-release-1.8.1/googletest/src
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${PROJECT_SOURCE_DIR}/include
                    ${PROJECT_SOURCE_DIR}/src)

file(GLOB sources "*.c" "*.cpp")

set (dependencies_sources
             "${PROJECT_SOURCE_DIR}/src/common/log/log.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/host_communicator/host_communicator.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/spinlock.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/kvs.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_empty_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_empty_coll_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_app_api_comm_split_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_app_api_coll_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/coll/coll_common_attributes.cpp"
             "${PROJECT_SOURCE_DIR}/src/coll/ccl_allgather_op_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/coll/ccl_allreduce_op_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/coll/ccl_alltoall_op_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/coll/ccl_alltoallv_op_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/coll/ccl_bcast_op_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/coll/ccl_reduce_op_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/coll/ccl_reduce_scatter_op_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/coll/ccl_sparse_allreduce_op_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/coll/ccl_barrier_op_attr.cpp"
)

set(CCL_MAJOR_VERSION     "0")
set(CCL_MINOR_VERSION     "0")
set(CCL_UPDATE_VERSION    "0")
set(CCL_PRODUCT_STATUS    "epsilon")
string(TIMESTAMP CCL_PRODUCT_BUILD_DATE "%Y-%m-%dT %H:%M:%SZ")
set(CCL_PRODUCT_FULL "${CCL_PRODUCT_STATUS}-${CCL_MAJOR_VERSION}.${CCL_MINOR_VERSION}.${CCL_UPDATE_VERSION} ${CCL_PRODUCT_BUILD_DATE} ${VCS_INFO}")
configure_file(${PROJECT_SOURCE_DIR}/include/ccl_config.h.in "${CMAKE_CURRENT_BINARY_DIR}/ccl_config.h")



foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} ${src} ${dependencies_sources})
    add_dependencies(${executable} gtest)
    if(API_UT)
        if(COMPUTE_RUNTIME)
            target_include_directories(${executable} PRIVATE
                                                    $<TARGET_PROPERTY:${COMPUTE_RUNTIME_TARGET_NAME},INTERFACE_INCLUDE_DIRECTORIES>)
            set_target_properties(${executable} PROPERTIES
                                                    INTERFACE_INCLUDE_DIRECTORIES
                                                    $<TARGET_PROPERTY:${COMPUTE_RUNTIME_TARGET_NAME},INTERFACE_INCLUDE_DIRECTORIES>)

            target_link_libraries(${executable} PUBLIC ${COMPUTE_RUNTIME_TARGET_NAME})
        endif(COMPUTE_RUNTIME)
    endif(API_UT)
    target_link_libraries(${executable} PUBLIC rt)
    target_link_libraries(${executable} PUBLIC m)
    target_link_libraries(${executable} PUBLIC dl)
    target_link_libraries(${executable} PUBLIC pthread)
    target_link_libraries(${executable} PUBLIC stdc++)
    target_link_libraries(${executable} PUBLIC gtest_main)
    target_link_libraries(${executable} PUBLIC gtest)
endforeach()
