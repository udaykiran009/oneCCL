set(sources
    allgatherv_vector.cpp
    allreduce_events.cpp
    allreduce_events_inorder.cpp
    alltoallv_empty_count.cpp
    allreduce_bf16_gpu.cpp
    close_ipc_handle_after_mem_free.cpp
    multi_comms.cpp)

set(CCL_INSTALL_TESTS "${CMAKE_CURRENT_SOURCE_DIR}")

set(REG_TESTS_DIR ${CCL_INSTALL_TESTS}/..)
set(REG_TESTS_INC_DIR ${REG_TESTS_DIR}/include)

message(STATUS "REG_TESTS_INC_DIR: ${REG_TESTS_INC_DIR}")

link_directories(${EXAMPLES_LIB_DIRS})

foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} ${src})
    target_link_libraries(${executable} PRIVATE ccl)
    target_link_libraries(${executable} PRIVATE -L${I_MPI_ROOT}/lib/release/)
    target_link_libraries(${executable} PRIVATE mpi)
    target_include_directories(${executable} PRIVATE ${EXAMPLES_INC_DIRS})
    target_include_directories(${executable} PRIVATE ${REG_TESTS_INC_DIR})
    install(TARGETS ${executable} RUNTIME DESTINATION ${CCL_INSTALL_TESTS} OPTIONAL)
endforeach(src ${sources})
