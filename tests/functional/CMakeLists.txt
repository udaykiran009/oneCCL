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
set(CCL_INSTALL_TESTS "$ENV{PWD}")
ADD_SUBDIRECTORY (googletest-release-1.8.1/googletest/)
include_directories(${PROJECT_SOURCE_DIR}/tests/googletest-release-1.8.1/googletest/include ${PROJECT_SOURCE_DIR}/tests/googletest-release-1.8.1/googletest/src)
include_directories($ENV{CCL_ROOT}/include)
link_directories($ENV{CCL_ROOT}/lib)
if (${CMAKE_VERSION} VERSION_LESS 3.1)
#cmake version below 3.1 does not support CMAKE_C[XX}_STANDARD flags
#set manually
    set(CXX_COMPILER_FLAGS "-std=c++11")
    set(C_COMPILER_FLAGS "-std=gnu99")
endif()
if(${CMAKE_C_COMPILER_ID} STREQUAL "Intel" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
	if (USE_CODECOV_FLAGS)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -prof-gen=srcpos -prof-src-root-cwd")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -prof-gen=srcpos -prof-src-root-cwd")
	endif()
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_COMPILER_FLAGS} -Wall -Werror -D_GNU_SOURCE -fvisibility=internal")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${C_COMPILER_FLAGS} -O0 -g -DENABLE_DEBUG=1")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${C_COMPILER_FLAGS} -O3")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${C_COMPILER_FLAGS} -O2 -g")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_COMPILER_FLAGS} -Wall -Werror -Wno-unneeded-internal-declaration -Wno-mismatched-tags -D_GNU_SOURCE -fvisibility=internal")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_COMPILER_FLAGS} -O0 -g -DENABLE_DEBUG=1")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_COMPILER_FLAGS} -O3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${CXX_COMPILER_FLAGS} -O2 -g")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(${CMAKE_C_COMPILER_ID} STREQUAL "Clang" AND ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(CMAKE_CLANG_FLAGS "-Wno-defaulted-function-deleted -Wno-mismatched-tags -Wno-unused-private-field -Wno-unneeded-internal-declaration -Wno-delete-non-abstract-non-virtual-dtor -Wno-reorder -DENABLE_SYCL=1 -D__STRICT_ANSI__ -fsycl")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CLANG_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CLANG_FLAGS} -std=c++11")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lsycl -lstdc++")
endif()

if(${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
    if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
        #c++17 introduces algined new operator, use it
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -faligned-new")
    endif()
endif()

foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} ${src})
    add_dependencies(${executable} gtest)
    add_dependencies(${executable} ccl)
    target_link_libraries(${executable}  PUBLIC gtest_main)
    target_link_libraries(${executable} PUBLIC gtest )
    target_link_libraries(${executable} PUBLIC rt)
    target_link_libraries(${executable} PUBLIC m)
    target_link_libraries(${executable} PRIVATE ccl)
    install(TARGETS ${executable} RUNTIME DESTINATION ${CCL_INSTALL_TESTS})
	# FIXME: enable allreduce_custom for non-direct case only
	if(NOT ${executable} STREQUAL "allreduce_custom_test")
		add_test (NAME ${executable} CONFIGURATIONS Default COMMAND mpiexec.hydra -l -n 2 -ppn 1 ${CCL_INSTALL_TESTS}/${executable} --gtest_output=xml:${CCL_INSTALL_TESTS}/${executable}_default_report.junit.xml)
    endif()
endforeach()

foreach(algo direct; naive; flat; multi_bcast)
add_test (NAME allgatherv_test_mpi_${algo} CONFIGURATIONS mpi_allgatherv_${algo} COMMAND mpiexec.hydra -l -n 2 -ppn 1 ${CCL_INSTALL_TESTS}/allgatherv_test --gtest_output=xml:${CCL_INSTALL_TESTS}/allgatherv_test_mpi_runtime_adjust_${algo}_report.junit.xml)
endforeach()

foreach(algo direct; tree; starlike; ring; ring_rma; double_tree; recursive_doubling)
add_test (NAME allreduce_test_mpi_${algo} CONFIGURATIONS mpi_allreduce_${algo} COMMAND mpiexec.hydra -l -n 2 -ppn 1 ${CCL_INSTALL_TESTS}/allreduce_test --gtest_output=xml:${CCL_INSTALL_TESTS}/allreduce_test_mpi_runtime_adjust_${algo}_report.junit.xml)
endforeach()

add_test (NAME allreduce_custom CONFIGURATIONS Custom COMMAND mpiexec.hydra -l -n 2 -ppn 1 ${CCL_INSTALL_TESTS}/allreduce_custom_test --gtest_output=xml:${CCL_INSTALL_TESTS}/allreduce_custom_report.junit.xml)

foreach(algo direct; ring; double_tree; naive)
add_test (NAME bcast_test_mpi_${algo} CONFIGURATIONS mpi_bcast_${algo} COMMAND mpiexec.hydra -l -n 2 -ppn 1 ${CCL_INSTALL_TESTS}/bcast_test --gtest_output=xml:${CCL_INSTALL_TESTS}/bcast_test_mpi_runtime_adjust_${algo}_report.junit.xml)
endforeach()

foreach(algo direct; tree; double_tree)
add_test (NAME reduce_test_mpi_${algo} CONFIGURATIONS mpi_reduce_${algo} COMMAND mpiexec.hydra -l -n 2 -ppn 1 ${CCL_INSTALL_TESTS}/reduce_test --gtest_output=xml:${CCL_INSTALL_TESTS}/reduce_test_mpi_runtime_adjust_${algo}_report.junit.xml)
endforeach()
