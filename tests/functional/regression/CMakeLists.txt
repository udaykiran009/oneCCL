set(sources alltoallv_empty_count.cpp)

set(CCL_INSTALL_TESTS "${CMAKE_CURRENT_BINARY_DIR}")

message(WARNING $"TEST DIR: ${CCL_INSTALL_TESTS}")

foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} ${src})
    target_link_libraries(${executable} PRIVATE ccl gtest_main gtest mpi)

    install(TARGETS ${executable} RUNTIME DESTINATION ${CCL_INSTALL_TESTS} OPTIONAL)
    add_test (NAME ${executable} CONFIGURATIONS regression COMMAND mpiexec.hydra -l -n 3 -ppn 1 ${CCL_INSTALL_TESTS}/${executable} --gtest_output=xml:${CCL_INSTALL_TESTS}/${executable}_default_report.junit.xml)

endforeach(src ${sources})
