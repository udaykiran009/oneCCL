set (executable "device_topology_suite")

enable_testing()

set(PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

include_directories(${PROJECT_SOURCE_DIR}/tests/googletest-release-1.8.1/googletest/include
                    ${PROJECT_SOURCE_DIR}/tests/googletest-release-1.8.1/googletest/src
                    ${CMAKE_CURRENT_BINARY_DIR}/..
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${PROJECT_SOURCE_DIR}/include
                    ${PROJECT_SOURCE_DIR}/src
                    ${PROJECT_SOURCE_DIR}/src/atl
                    ${PROJECT_SOURCE_DIR}/tests/unit/include)

file(GLOB sources "*.c" "*.cpp")
set (dependencies_sources
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/topology/topology_construction_utils.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/topology/topology_serializer.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/topology/ring/device_group_ring_creator.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/topology/ring/thread_group_ring_creator.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/topology/ring/process_group_ring_creator.cpp"

             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/context/device_group_ctx.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/context/device_storage.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/context/thread_group_ctx.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/context/process_group_ctx.cpp"

             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/devices/ccl_gpu_comm.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/devices/ccl_ipc_gpu_comm.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/devices/ccl_virtual_gpu_comm.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/devices/ccl_gpu_base_comm.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/devices/communication_structs/connection.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/devices/communication_structs/ipc_connection.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/devices/communication_structs/ipc_server.cpp"
             "${PROJECT_SOURCE_DIR}/src/common/comm/l0/devices/communication_structs/ipc_client.cpp"

             "${PROJECT_SOURCE_DIR}/tests/unit/l0/stubs/modules/module_source_data.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/ipc_ctx_session.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/l0/stubs/modules/base_entry_module.cpp"
             "${PROJECT_SOURCE_DIR}/tests/unit/l0/stubs/modules/kernel_utils.cpp"
             )

list(APPEND dependencies_sources ${specific_sources})

foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} $<TARGET_OBJECTS:napi_objects>
                                 $<TARGET_OBJECTS:api_objects>
                                 $<TARGET_OBJECTS:core_objects>
                                 $<TARGET_OBJECTS:test_stub_core_objects>
                                 ${src} ${dependencies_sources})
    add_dependencies(${executable} gtest)


    target_link_libraries(${executable} PUBLIC interface_library)
endforeach()
