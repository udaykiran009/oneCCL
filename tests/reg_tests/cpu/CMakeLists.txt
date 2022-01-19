set(sources
    allgatherv.cpp
    allreduce.cpp
    alltoallv.cpp
    broadcast.cpp
    communicator.cpp
    custom_allreduce.cpp
    datatype.cpp
    external_kvs.cpp
    priority_allreduce.cpp
    reduce.cpp
    reduce_scatter.cpp
    recreate_comm.cpp
    unordered_allreduce.cpp)

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
