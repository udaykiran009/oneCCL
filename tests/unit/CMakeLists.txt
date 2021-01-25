if(MULTI_GPU_SUPPORT)
    set(UPSTREAM_SOURCE_DIR "${PROJECT_SOURCE_DIR}")
    include(${UPSTREAM_SOURCE_DIR}/cmake/helpers.cmake)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    if (COMPUTE_BACKEND)
        activate_compute_backend("${UPSTREAM_SOURCE_DIR}/cmake" ${COMPUTE_BACKEND})

        if (${CCL_ENABLE_SYCL_V} STREQUAL 1)
            option (CCL_ENABLE_SYCL "Enable CCL SYCL runtime" ON)
            message(STATUS "Enable CCL SYCL runtime")
        endif()

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPUTE_BACKEND_FLAGS}")
        add_definitions(${DEFINITIONS})

        if ((${COMPUTE_BACKEND_TARGET_NAME} STREQUAL "ze_loader")
                  OR (${COMPUTE_BACKEND_TARGET_NAME} STREQUAL "Intel::SYCL_level_zero"))
            option(MULTI_GPU_SUPPORT "Enable multi GPU support" ON)
            set(MULTI_GPU_SUPPORT ON)
            activate_compute_backend("${UPSTREAM_SOURCE_DIR}/cmake" L0)
            message ("Enable multi GPU support: ${MULTI_GPU_SUPPORT}")
            message ("COMPUTE_BACKEND_TARGET_NAME: ${COMPUTE_BACKEND_TARGET_NAME}")
        endif()
    endif(COMPUTE_BACKEND)

    list( APPEND napi_wrapper_sources
                 "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/base.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/device.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/driver.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/export.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/context.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/platform.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/subdevice.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/primitives.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/utils.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/native_device_api/interop_utils.cpp")

    list( APPEND api_sources
                 "${UPSTREAM_SOURCE_DIR}/src/ccl_cpp_utils.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/ccl_empty_attr.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/ccl_empty_coll_attr.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/ccl_app_api_comm_split_attr.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/ccl_app_api_coll_attr.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/ccl_app_api_event.cpp"

                 #"${UPSTREAM_SOURCE_DIR}/src/ccl_cpp_stream.cpp"
                 #"${UPSTREAM_SOURCE_DIR}/src/ccl_app_api_datatype_attr.cpp"
                 #"${UPSTREAM_SOURCE_DIR}/src/ccl_empty_stream.cpp"
                 )
    list (APPEND core_sources
                 "${UPSTREAM_SOURCE_DIR}/src/coll/coll_common_attributes.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/coll/ccl_allgather_op_attr.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/coll/ccl_allreduce_op_attr.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/coll/ccl_alltoall_op_attr.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/coll/ccl_alltoallv_op_attr.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/coll/ccl_bcast_op_attr.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/coll/ccl_reduce_op_attr.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/coll/ccl_reduce_scatter_op_attr.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/coll/ccl_sparse_allreduce_op_attr.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/coll/ccl_barrier_op_attr.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/common/log/log.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/common/event/ccl_event.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/common/event/impls/native_event.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/common/stream/stream.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/common/comm/comm.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/common/utils/version.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/common/comm/l0/context_comm_addr.cpp"
                 "${UPSTREAM_SOURCE_DIR}/src/common/request/request.cpp"
                 )
    list (APPEND test_stub_core_sources
                 "${UPSTREAM_SOURCE_DIR}/tests/unit/api/stubs/spinlock.cpp"
                 "${UPSTREAM_SOURCE_DIR}/tests/unit/api/stubs/kvs.cpp"
                 "${UPSTREAM_SOURCE_DIR}/tests/unit/api/stubs/host_communicator.cpp"
                 "${UPSTREAM_SOURCE_DIR}/tests/unit/api/stubs/user_kvs.cpp"
                 "${UPSTREAM_SOURCE_DIR}/tests/unit/api/stubs/global.cpp"
                 "${UPSTREAM_SOURCE_DIR}/tests/unit/api/stubs/env.cpp"
                 "${UPSTREAM_SOURCE_DIR}/tests/unit/api/stubs/cache.cpp"
                 "${UPSTREAM_SOURCE_DIR}/tests/unit/api/stubs/datatype.cpp"
                 "${UPSTREAM_SOURCE_DIR}/tests/unit/api/stubs/native_runtime_utils.cpp"
                 "${UPSTREAM_SOURCE_DIR}/tests/unit/api/stubs/atl_wrapper.cpp"
                 "${UPSTREAM_SOURCE_DIR}/tests/unit/api/stubs/ccl_unordered_coll_manager.cpp"
                 "${UPSTREAM_SOURCE_DIR}/tests/unit/api/stubs/ccl_allreduce_2d_builder.cpp"
                 "${UPSTREAM_SOURCE_DIR}/tests/unit/l0/stubs/sched/sched.cpp"
                 #"${UPSTREAM_SOURCE_DIR}/tests/unit/l0/stubs/sched/sched_base.cpp"
                 "${UPSTREAM_SOURCE_DIR}/tests/unit/l0/stubs/sched/gpu_sched.cpp"
                 "${UPSTREAM_SOURCE_DIR}/tests/unit/l0/stubs/sched/gpu_concurrent_sched.cpp"
                 "${UPSTREAM_SOURCE_DIR}/tests/unit/l0/stubs/spare/bfp16.cpp"
                 )
    list( APPEND napi_wrapper_sources "${UPSTREAM_SOURCE_DIR}/src/native_device_api/empty/export.cpp")

    set(CCL_MAJOR_VERSION     "0")
    set(CCL_MINOR_VERSION     "0")
    set(CCL_UPDATE_VERSION    "0")
    set(CCL_PRODUCT_STATUS    "epsilon")
    string(TIMESTAMP CCL_PRODUCT_BUILD_DATE "%Y-%m-%dT %H:%M:%SZ")
    set(CCL_PRODUCT_FULL "${CCL_PRODUCT_STATUS}-${CCL_MAJOR_VERSION}.${CCL_MINOR_VERSION}.${CCL_UPDATE_VERSION} ${CCL_PRODUCT_BUILD_DATE} ${VCS_INFO}")
    configure_file(${UPSTREAM_SOURCE_DIR}/include/oneapi/ccl/config.h.in "${CMAKE_CURRENT_BINARY_DIR}/oneapi/ccl/config.h")



    #add_subdirectory (${UPSTREAM_SOURCE_DIR}/tests/functional/googletest-release-1.8.1/googletest/ ${PROJECT_SOURCE_DIR}/tests/units)


    # prepare interface library: dependencies only
    add_library(interface_library INTERFACE)
    target_include_directories(interface_library INTERFACE
                               ${UPSTREAM_SOURCE_DIR}/tests/functional/googletest-release-1.8.1/googletest/include
                               ${UPSTREAM_SOURCE_DIR}/tests/functional/googletest-release-1.8.1/googletest/src
                               ${CMAKE_CURRENT_BINARY_DIR}
                               ${UPSTREAM_SOURCE_DIR}/include
                               ${UPSTREAM_SOURCE_DIR}/src
                               ${UPSTREAM_SOURCE_DIR}/src/atl)
    if (COMPUTE_BACKEND)
        target_include_directories(interface_library INTERFACE
                                                $<TARGET_PROPERTY:${COMPUTE_BACKEND_TARGET_NAME},INTERFACE_INCLUDE_DIRECTORIES>)
        target_link_libraries(interface_library INTERFACE ${COMPUTE_BACKEND_TARGET_NAME})
    endif(COMPUTE_BACKEND)
    target_link_libraries(interface_library INTERFACE rt m dl pthread stdc++ gtest_main gtest)

    # prepare component targets
    add_library(napi_objects OBJECT ${napi_wrapper_sources})
    add_library(api_objects OBJECT ${api_sources})
    add_library(core_objects OBJECT ${core_sources})
    add_library(test_stub_core_objects OBJECT ${test_stub_core_sources})


    if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
        set_target_properties(napi_objects PROPERTIES INTERFACE_LINK_LIBRARIES interface_library)
        target_include_directories(napi_objects PUBLIC $<TARGET_PROPERTY:interface_library,INTERFACE_INCLUDE_DIRECTORIES>)

        set_target_properties(api_objects PROPERTIES INTERFACE_LINK_LIBRARIES interface_library)
        target_include_directories(api_objects PUBLIC $<TARGET_PROPERTY:interface_library,INTERFACE_INCLUDE_DIRECTORIES>)

        set_target_properties(core_objects PROPERTIES INTERFACE_LINK_LIBRARIES interface_library)
        target_include_directories(core_objects PUBLIC $<TARGET_PROPERTY:interface_library,INTERFACE_INCLUDE_DIRECTORIES>)

        set_target_properties(test_stub_core_objects PROPERTIES INTERFACE_LINK_LIBRARIES interface_library)
        target_include_directories(test_stub_core_objects PUBLIC $<TARGET_PROPERTY:interface_library,INTERFACE_INCLUDE_DIRECTORIES>)
    else()
        target_link_libraries(napi_objects interface_library)
        target_link_libraries(api_objects interface_library)
        target_link_libraries(core_objects interface_library)
        target_link_libraries(test_stub_core_objects interface_library)
    endif()

    # include UTs
    add_subdirectory(kernels)
    add_subdirectory(device_topology)
    add_subdirectory(native_api)
endif()
add_subdirectory(api)

# TODO:
# l0 folder is not compiled.
# Have to disband this folder into independent essences.
# like: api, device_topology, kernels and etc.
# add_subdirectory(l0)
