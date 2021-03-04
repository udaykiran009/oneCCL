set (executable "coll_attr_suite")

enable_testing()

set(PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}")

include_directories(${CMAKE_CURRENT_BINARY_DIR}/..
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${PROJECT_SOURCE_DIR}/include
                    ${PROJECT_SOURCE_DIR}/src
                    ${PROJECT_SOURCE_DIR}/src/atl)

file(GLOB sources "*.c" "*.cpp")

set (dependencies_sources
             "${PROJECT_SOURCE_DIR}/src/ccl_empty_attr.cpp"
             "${PROJECT_SOURCE_DIR}/src/ccl_empty_coll_attr.cpp"
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
             "${PROJECT_SOURCE_DIR}/src/common/utils/version.cpp")

foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} ${src} ${dependencies_sources})
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
