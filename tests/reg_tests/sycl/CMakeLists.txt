set(sources
    allgatherv_vector.cpp
    allreduce_events.cpp
    alltoallv_empty_count.cpp)

set(CCL_INSTALL_TESTS "${CMAKE_CURRENT_SOURCE_DIR}")

foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)

    list(FIND EXCLUDE_TESTS ${executable} _index)
    if(${_index} GREATER -1)
        continue()
    endif()

    add_executable(${executable} ${src})
    target_link_libraries(${executable} PRIVATE ccl mpi)
    target_include_directories(${executable} PRIVATE ${EXAMPLES_INC_DIRS})
    install(TARGETS ${executable} RUNTIME DESTINATION ${CCL_INSTALL_TESTS} OPTIONAL)
endforeach(src ${sources})
