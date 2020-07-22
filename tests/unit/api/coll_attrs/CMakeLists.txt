set (executable "coll_attr_suite")

enable_testing()
include_directories(${PROJECT_SOURCE_DIR}/tests/googletest-release-1.8.1/googletest/include
                    ${PROJECT_SOURCE_DIR}/tests/googletest-release-1.8.1/googletest/src
                    ${PROJECT_SOURCE_DIR}/include
                    ${PROJECT_SOURCE_DIR}/src)

file(GLOB sources "*.c" "*.cpp")

list (APPEND sources
            "${PROJECT_SOURCE_DIR}/src/ccp_cpp_api.cpp")

foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} ${src})
    target_link_libraries(${executable} PUBLIC rt)
    target_link_libraries(${executable} PUBLIC m)
    target_link_libraries(${executable} PUBLIC dl)
    target_link_libraries(${executable} PUBLIC pthread)
    target_link_libraries(${executable} PUBLIC stdc++)
endforeach()
