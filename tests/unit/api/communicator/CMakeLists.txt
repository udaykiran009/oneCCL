set (executable "device_communicator_suite")


enable_testing()
message ("PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")

if(API_UT)
    set(PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/../..")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

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
             "${PROJECT_SOURCE_DIR}/src/ccl_empty_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_empty_coll_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_empty_stream.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_app_api_comm_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_app_api_init_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_app_api_kvs_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_app_api_comm_split_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_app_api_coll_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_cpp_stream.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_cpp_communicator.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_cpp_environment.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_app_api_datatype_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_cpp_device.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_cpp_context.cpp"
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
             "${PROJECT_SOURCE_DIR}/src/common/log/log.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/device/device.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_cpp_device.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/context/context.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_cpp_context.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/event/ccl_event.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/event/impls/native_event.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/stream/stream.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/comm.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/compiler_comm_interface_dispatcher.cpp"
#             "${PROJECT_SOURCE_DIR}/src/native_device_api/interop_utils.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_cpp_utils.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/spinlock.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/kvs.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_app_api_event.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/host_communicator.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/user_kvs.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/global.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/env.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/cache.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/datatype.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/native_runtime_utils.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/atl_wrapper.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/ccl_unordered_coll_manager.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/utils/version.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/ccl_allreduce_2d_builder.cpp"
)

set(specific_sources)
if(MULTI_GPU_SUPPORT)
    set(specific_sources
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/comm_context_storage.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/comm_context.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/gpu_comm_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/context_comm_addr.cpp"

             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs//ipc_ctx_session.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/context/scaling_ctx/ipc_session_key.cpp"

             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/single_device_communicator.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/device_topology_communicator.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/thread_topology_communicator.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/process_topology_communicator.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/process_group_context.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/thread_group_context.cpp")
elseif(CCL_ENABLE_SYCL)
    set(specific_sources "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/single_device_communicator.cpp")
endif()

list(APPEND dependencies_sources ${specific_sources})

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
