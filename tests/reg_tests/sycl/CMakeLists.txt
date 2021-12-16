set(sources
    allgatherv_vector.cpp
    allreduce_bf16_gpu.cpp
    allreduce_events.cpp
    allreduce_events_inorder.cpp
    alltoallv_empty_count.cpp
    close_ipc_handle_after_mem_free.cpp
    catch_comm_exception.cpp
    multi_comms.cpp
    stub_backend.cpp)

set(CCL_INSTALL_TESTS "${CMAKE_CURRENT_SOURCE_DIR}")

foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} ${src})
    target_link_libraries(${executable} PUBLIC ccl)
    target_link_libraries(${executable} PUBLIC -L${I_MPI_ROOT}/lib/release/)
    target_link_libraries(${executable} PUBLIC mpi)
    target_link_libraries(${executable} PUBLIC -L${LIBFABRIC_LIB_DIR})
    target_link_libraries(${executable} PUBLIC fabric)
    target_link_libraries(${executable} PUBLIC ${COMPUTE_BACKEND_TARGET_NAME})
    install(TARGETS ${executable} RUNTIME DESTINATION ${CCL_INSTALL_TESTS} OPTIONAL)
endforeach(src ${sources})
