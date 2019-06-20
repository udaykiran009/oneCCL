cmake_minimum_required (VERSION 2.8)
file(GLOB sources "*.c" "*.cpp")
################################
# GTest
################################
ADD_SUBDIRECTORY (googletest-release-1.8.1/googletest/)
include_directories(${PROJECT_SOURCE_DIR}/tests/googletest-release-1.8.1/googletest/include ${PROJECT_SOURCE_DIR}/tests/googletest-release-1.8.1/googletest/src)
foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} ${src})
    add_dependencies(${executable} gtest)
    target_link_libraries(${executable}  PUBLIC gtest_main)
    target_link_libraries(${executable} PUBLIC gtest )
    target_link_libraries(${executable} PUBLIC rt)
    target_link_libraries(${executable} PUBLIC m)
    target_link_libraries(${executable} PRIVATE mlsl2)
    install(TARGETS ${executable} RUNTIME DESTINATION ${MLSL_INSTALL_TESTS})
    add_test (NAME ${executable} CONFIGURATIONS Default COMMAND mpiexec.hydra -l -n 2 -ppn 1 ${MLSL_INSTALL_TESTS}/${executable} --gtest_output=xml:${MLSL_INSTALL_TESTS}/${executable}_default_report.junit.xml)
    add_test (NAME ${executable}_mpi CONFIGURATIONS Mpi COMMAND mpiexec.hydra -l -n 2 -ppn 1 ${MLSL_INSTALL_TESTS}/${executable} --gtest_output=xml:${MLSL_INSTALL_TESTS}/${executable}_mpi_runtime_report.junit.xml)
endforeach()
foreach(algo ring; double_tree; whole)
add_test (NAME bcast_test_mpi_${algo} CONFIGURATIONS mpi_bcast_${algo} COMMAND mpiexec.hydra -l -n 2 -ppn 1 ${MLSL_INSTALL_TESTS}/bcast_test --gtest_output=xml:${MLSL_INSTALL_TESTS}/bcast_test_mpi_runtime_adjust_${algo}_report.junit.xml)
endforeach()

foreach(algo tree; double_tree; whole)
add_test (NAME reduce_test_mpi_${algo} CONFIGURATIONS mpi_reduce_${algo} COMMAND mpiexec.hydra -l -n 2 -ppn 1 ${MLSL_INSTALL_TESTS}/reduce_test --gtest_output=xml:${MLSL_INSTALL_TESTS}/reduce_test_mpi_runtime_adjust_${algo}_report.junit.xml)
endforeach()

foreach(algo tree; starlike; ring; ring_rma; whole; double_tree)
add_test (NAME allreduce_test_mpi_${algo} CONFIGURATIONS mpi_allreduce_${algo} COMMAND mpiexec.hydra -l -n 2 -ppn 1 ${MLSL_INSTALL_TESTS}/allreduce_test --gtest_output=xml:${MLSL_INSTALL_TESTS}/allreduce_test_mpi_runtime_adjust_${algo}_report.junit.xml)
endforeach()

foreach(algo naive; whole)
add_test (NAME allgatherv_test_mpi_${algo} CONFIGURATIONS mpi_allgatherv_${algo} COMMAND mpiexec.hydra -l -n 2 -ppn 1 ${MLSL_INSTALL_TESTS}/allgatherv_test --gtest_output=xml:${MLSL_INSTALL_TESTS}/allgatherv_test_mpi_runtime_adjust_${algo}_report.junit.xml)
endforeach()

add_test (NAME allreduce_custom CONFIGURATIONS Custom COMMAND mpiexec.hydra -l -n 2 -ppn 1 ${MLSL_INSTALL_TESTS}/allreduce_custom_test --gtest_output=xml:${MLSL_INSTALL_TESTS}/allreduce_custom_report.junit.xml)
