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
    add_test (NAME ${executable} COMMAND mpiexec.hydra -l -n 2 -ppn 1 ${MLSL_INSTALL_TESTS}/${executable} --gtest_output=xml:${MLSL_INSTALL_TESTS}/${executable}_report.junit.xml)
endforeach()
