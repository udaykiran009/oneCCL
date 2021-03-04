set (executable "multi_device_suite")

enable_testing()

set(PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

include_directories(${PROJECT_SOURCE_DIR}/tests/functional/googletest-release-1.8.1/googletest/include
                    ${PROJECT_SOURCE_DIR}/tests/functional/googletest-release-1.8.1/googletest/src
                    ${CMAKE_CURRENT_BINARY_DIR}/..
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${PROJECT_SOURCE_DIR}/include
                    ${PROJECT_SOURCE_DIR}/src
                    ${PROJECT_SOURCE_DIR}/tests/unit/include
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
                                 ${src} ${dependencies_sources})
    add_dependencies(${executable} gtest)
    add_test (NAME ${executable}_default CONFIGURATIONS MultiDeviceDefaultSuite COMMAND ${CCL_UNIT_TESTS_BUILD}/kernels/multi_device/${executable} --gtest_output=xml:${CCL_UNIT_TESTS_BUILD}/kernels/multi_device/${executable}_default_suite_report.junit.xml)
    add_test (NAME ${executable}_test CONFIGURATIONS ${executable}_test COMMAND ${CCL_UNIT_TESTS_BUILD}/kernels/multi_device/${executable} --gtest_output=xml:${CCL_UNIT_TESTS_BUILD}/kernels/multi_device/${executable}_test_report.junit.xml)
    target_link_libraries(${executable} PUBLIC interface_library)
endforeach()
