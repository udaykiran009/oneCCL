set (executable "single_dev")

enable_testing()

if(L0_TESTS_UT)
    set(PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

include_directories(${PROJECT_SOURCE_DIR}/tests/googletest-release-1.8.1/googletest/include
                    ${PROJECT_SOURCE_DIR}/tests/googletest-release-1.8.1/googletest/src
                    ${CMAKE_CURRENT_BINARY_DIR}/..
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${PROJECT_SOURCE_DIR}/include
                    ${PROJECT_SOURCE_DIR}/src
                    ${PROJECT_SOURCE_DIR}/tests/unit/include
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


    target_link_libraries(${executable} PUBLIC interface_library)
endforeach()
