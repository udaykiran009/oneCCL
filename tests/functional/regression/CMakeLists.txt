set(sources
    alltoallv_empty_count.cpp
    allgatherv_vector.cpp)

set(CCL_INSTALL_TESTS "${CMAKE_CURRENT_BINARY_DIR}")

foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} ${src})
    target_link_libraries(${executable} PRIVATE ccl gtest_main gtest mpi)
    install(TARGETS ${executable} RUNTIME DESTINATION ${CCL_INSTALL_TESTS} OPTIONAL)
endforeach(src ${sources})

add_test(NAME alltoallv_empty_count CONFIGURATIONS regression COMMAND mpiexec.hydra -l -n 3 -ppn 1 ${CCL_INSTALL_TESTS}/alltoallv_empty_count --gtest_output=xml:${CCL_INSTALL_TESTS}/alltoallv_empty_count_default_report.junit.xml)

add_test(NAME allgatherv_vector CONFIGURATIONS regression COMMAND mpiexec.hydra -l -n 4 -ppn 1 ${CCL_INSTALL_TESTS}/allgatherv_vector --gtest_output=xml:${CCL_INSTALL_TESTS}/allgatherv_vector_default_report.junit.xml)
