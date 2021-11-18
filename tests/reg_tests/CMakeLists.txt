cmake_minimum_required (VERSION 2.8)

if (POLICY CMP0048)
    cmake_policy(SET CMP0048 OLD)
endif (POLICY CMP0048)

set(PROJECT_NAME "oneCCL regression tests")
project(${PROJECT_NAME})

message(STATUS "PROJECT_NAME: ${PROJECT_NAME}")
message(STATUS "PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")

set(EXAMPLES_DIR ${PROJECT_SOURCE_DIR}/../../examples)
set(EXAMPLES_INC_DIRS ${EXAMPLES_DIR}/include)
message(STATUS "EXAMPLES_INC_DIRS: ${EXAMPLES_INC_DIRS}")
include_directories(${EXAMPLES_INC_DIRS})
link_directories(${EXAMPLES_LIB_DIRS})

set(REG_TESTS_DIR ${PROJECT_SOURCE_DIR})
set(REG_TESTS_INC_DIR ${REG_TESTS_DIR}/include)
message(STATUS "REG_TESTS_INC_DIR: ${REG_TESTS_INC_DIR}")
include_directories(${REG_TESTS_INC_DIR})

if (DEFINED ENV{I_MPI_ROOT})
    set(I_MPI_ROOT "$ENV{I_MPI_ROOT}")
endif()

if (DEFINED ENV{CCL_ROOT})
    set(CCL_ROOT "$ENV{CCL_ROOT}")
endif()
message(STATUS "CCL_ROOT: ${CCL_ROOT}")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (${CMAKE_C_COMPILER_ID} STREQUAL "Clang" AND ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR(${CMAKE_CXX_COMPILER_ID} STREQUAL "IntelLLVM"))
    set(CMAKE_CXX_STANDARD 17)
endif()

# only enable common tests if we build in standalone mode
# as they require ccl package
if (${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    add_subdirectory(common)
endif()

add_subdirectory(cpu)

if ("${COMPUTE_BACKEND}" STREQUAL "dpcpp_level_zero")
    add_subdirectory(sycl)
endif()
