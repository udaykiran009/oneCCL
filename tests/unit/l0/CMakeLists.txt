cmake_minimum_required (VERSION 2.8)
file(GLOB sources "*.c" "*.cpp")
################################
# GTest
################################
#show build info
message(STATUS "Installation directory: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C compiler : ${CMAKE_C_COMPILER}")
message(STATUS "CXX compiler : ${CMAKE_CXX_COMPILER}")

enable_testing()
set(CCL_INSTALL_UNIT_TESTS "$ENV{PWD}")
include_directories(${PROJECT_SOURCE_DIR}/tests/googletest-release-1.8.1/googletest/include
                    ${PROJECT_SOURCE_DIR}/tests/googletest-release-1.8.1/googletest/src
                    ${PROJECT_SOURCE_DIR}/include
                    ${PROJECT_SOURCE_DIR}/src)

if (${CMAKE_VERSION} VERSION_LESS 3.1)
#cmake version below 3.1 does not support CMAKE_C[XX}_STANDARD flags
#set manually
    set(CXX_COMPILER_FLAGS "-std=c++11")
    set(C_COMPILER_FLAGS "-std=gnu99")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_COMPILER_FLAGS} -Wall -Werror -D_GNU_SOURCE -fvisibility=internal -DUNIT_TESTS")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${C_COMPILER_FLAGS} -O0 -g -DENABLE_DEBUG=1 -DUNIT_TESTS")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${C_COMPILER_FLAGS} -O3 -DUNIT_TESTS")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${C_COMPILER_FLAGS} -O2 -g -DUNIT_TESTS")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_COMPILER_FLAGS} -Wall -Werror -Wno-unneeded-internal-declaration -Wno-mismatched-tags -D_GNU_SOURCE -fvisibility=internal -DUNIT_TESTS")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_COMPILER_FLAGS} -O0 -g -DENABLE_DEBUG=1 -DUNIT_TESTS")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_COMPILER_FLAGS} -O3 -DUNIT_TESTS")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${CXX_COMPILER_FLAGS} -O2 -g -DUNIT_TESTS")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set (DEPENDENCIES gtest ccl-ut-static)

foreach(libs ${DEPENDENCIES})
    get_target_property(CCL_STATIC_PATH ccl-ut-static ARCHIVE_OUTPUT_DIRECTORY)
    get_target_property(UNIT_TEST_INCLUDE ccl INCLUDE_DIRECTORIES)

    link_directories($ENV{CCL_ROOT}/lib ${CCL_STATIC_PATH})
    include_directories(${UNIT_TEST_INCLUDE})
endforeach()

set(ADD_LIBS gtest_main
             gtest
             rt
             m
             libcclut.a
             level_zero
             dl)
foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} ${src})
    add_dependencies(${executable} ${DEPENDENCIES})
    link_directories("/usr/lib/level_zero/")
    target_link_libraries(${executable} ${ADD_LIBS})
#    install(TARGETS ${executable} RUNTIME DESTINATION ${CCL_INSTALL_UNIT_TESTS})
    #add_test (NAME ${executable} CONFIGURATIONS Default COMMAND mpiexec.hydra -l -n 2 -ppn 1 ${CCL_INSTALL_UNIT_TESTS}/${executable} --gtest_output=xml:${CCL_INSTALL_UNIT_TESTS}/${executable}_default_report.junit.xml)
    #add_test (NAME ${executable}_mpi CONFIGURATIONS Mpi COMMAND mpiexec.hydra -l -n 2 -ppn 1 ${CCL_INSTALL_UNIT_TESTS}/${executable} --gtest_output=xml:${CCL_INSTALL_UNIT_TESTS}/${executable}_mpi_runtime_report.junit.xml)
endforeach()

#add_test (NAME test_l0 CONFIGURATIONS Custom COMMAND mpiexec.hydra -l -n 2 -ppn 1 ${CCL_INSTALL_UNIT_TESTS}/test_l0 --gtest_output=xml:${CCL_INSTALL_UNIT_TESTS}/test_sched.junit.xml)
