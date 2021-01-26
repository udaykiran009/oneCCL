set (executable "single_device_suite")

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
                    ${PROJECT_SOURCE_DIR}/tests/unit/kernels
                    ${PROJECT_SOURCE_DIR}/tests/unit/kernels/include
                    ${PROJECT_SOURCE_DIR}/src/atl)

file(GLOB sources "*.c" "*.cpp")
foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} $<TARGET_OBJECTS:napi_objects>
                                 $<TARGET_OBJECTS:api_objects>
                                 $<TARGET_OBJECTS:core_objects>
                                 $<TARGET_OBJECTS:test_stub_core_objects>
                                 ${src})
    add_dependencies(${executable} gtest)
    add_test (NAME ${executable} CONFIGURATIONS Default COMMAND ${CCL_UNIT_TESTS_BUILD}/kernels/single_device/${executable} --gtest_output=xml:${CCL_UNIT_TESTS_BUILD}/kernels/single_device/${executable}_default_report.junit.xml)
    target_link_libraries(${executable} PUBLIC interface_library)
endforeach()

add_test (NAME ring_allgatherv_kernel_ut CONFIGURATIONS ring_allgatherv_kernel_ut COMMAND ${CCL_UNIT_TESTS_BUILD}/kernels/single_device/kernel_ring_single_device_suite  --gtest_filter=ring_allgatherv_single_device_fixture/* --gtest_output=xml:${CCL_UNIT_TESTS_BUILD}/kernels/single_device/allgatherv_kernel_unit_test_report.junit.xml)

add_test (NAME ring_allreduce_kernel_ut CONFIGURATIONS ring_allreduce_kernel_ut COMMAND ${CCL_UNIT_TESTS_BUILD}/kernels/single_device/kernel_ring_single_device_suite  --gtest_filter=ring_allreduce_single_device_fixture/* --gtest_output=xml:${CCL_UNIT_TESTS_BUILD}/kernels/single_device/allreduce_kernel_unit_test_report.junit.xml)

add_test (NAME ring_alltoallv_kernel_ut CONFIGURATIONS ring_alltoallv_kernel_ut COMMAND ${CCL_UNIT_TESTS_BUILD}/kernels/single_device/kernel_ring_single_device_suite  --gtest_filter=ring_alltoallv_single_device_fixture/* --gtest_output=xml:${CCL_UNIT_TESTS_BUILD}/kernels/single_device/alltoallv_kernel_unit_test_report.junit.xml)

add_test (NAME ring_bcast_kernel_ut CONFIGURATIONS ring_bcast_kernel_ut COMMAND ${CCL_UNIT_TESTS_BUILD}/kernels/single_device/kernel_ring_single_device_suite  --gtest_filter=ring_bcast_single_device_fixture/* --gtest_output=xml:${CCL_UNIT_TESTS_BUILD}/kernels/single_device/bcast_kernel_unit_test_report.junit.xml)

add_test (NAME ring_reduce_kernel_ut CONFIGURATIONS ring_reduce_kernel_ut COMMAND ${CCL_UNIT_TESTS_BUILD}/kernels/single_device/kernel_ring_single_device_suite  --gtest_filter=ring_reduce_single_device_fixture/* --gtest_output=xml:${CCL_UNIT_TESTS_BUILD}/kernels/single_device/reduce_kernel_unit_test_report.junit.xml)

add_test (NAME ring_reduce_scatter_kernel_ut CONFIGURATIONS ring_reduce_scatter_kernel_ut COMMAND ${CCL_UNIT_TESTS_BUILD}/kernels/single_device/kernel_ring_single_device_suite  --gtest_filter=ring_reduce_scatter_single_device_fixture/* --gtest_output=xml:${CCL_UNIT_TESTS_BUILD}/kernels/single_device/reduce_scatter_kernel_unit_test_report.junit.xml)
