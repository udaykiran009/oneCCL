set (executable "ip_serialization_suite")

enable_testing()
message ("PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")

set(PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

include_directories(${PROJECT_SOURCE_DIR}/tests/functional/googletest-release-1.8.1/googletest/include
                    ${PROJECT_SOURCE_DIR}/tests/functional/googletest-release-1.8.1/googletest/src
                    ${CMAKE_CURRENT_BINARY_DIR}/..
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${PROJECT_SOURCE_DIR}/include
                    ${PROJECT_SOURCE_DIR}/src
                    ${PROJECT_SOURCE_DIR}/src/atl
                    ${PROJECT_SOURCE_DIR}/tests/unit/include)

file(GLOB sources "*.c" "*.cpp")
set (dependencies_sources
            ${PROJECT_SOURCE_DIR}/src/ccl_cpp_utils.cpp
            ${PROJECT_SOURCE_DIR}/src/common/log/log.cpp
            ${PROJECT_SOURCE_DIR}/src/common/comm/l0/devices/communication_structs/connection.cpp
            ${PROJECT_SOURCE_DIR}/src/common/comm/l0/devices/communication_structs/ipc_connection.cpp
            ${PROJECT_SOURCE_DIR}/src/common/comm/l0/devices/communication_structs/ipc_server.cpp
            ${PROJECT_SOURCE_DIR}/src/common/comm/l0/devices/communication_structs/ipc_client.cpp
            ${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/spinlock.cpp
            ${PROJECT_SOURCE_DIR}/tests/unit/api/stubs/env.cpp
            ${PROJECT_SOURCE_DIR}/tests/unit/l0/stubs/spare/bfp16.cpp)

foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} $<TARGET_OBJECTS:napi_objects>
                                 ${src} ${dependencies_sources})
    add_dependencies(${executable} gtest)


    target_link_libraries(${executable} PUBLIC interface_library)
endforeach()
