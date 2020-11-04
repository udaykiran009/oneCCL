cmake_minimum_required (VERSION 2.8)

set(PROJECT_NAME "oneCCL internal examples")

project(${PROJECT_NAME})
include(${PROJECT_SOURCE_DIR}/../../cmake/helpers.cmake)

if (DEFINED ENV{CCL_CONFIGURATION})
    set(CCL_CONFIGURATION "$ENV{CCL_CONFIGURATION}")
    if(${CCL_CONFIGURATION} STREQUAL "cpu_gpu_dpcpp")
        set(COMPUTE_RUNTIME_TARGET_NAME "Intel::SYCL")
    endif()
endif()

if (DEFINED ENV{CCL_ROOT})
    set(CCL_ROOT "$ENV{CCL_ROOT}")
else()
    message(FATAL_ERROR "Please define CCL_ROOT environment variable")
endif()

if (DEFINED ENV{I_MPI_ROOT})
    set(I_MPI_ROOT "$ENV{I_MPI_ROOT}")
else()
    message(FATAL_ERROR "Please define I_MPI_ROOT environment variable")
endif()

message(STATUS "CCL_ROOT: ${CCL_ROOT}")
message(STATUS "I_MPI_ROOT: ${I_MPI_ROOT}")
message(STATUS "CCL_CONFIGURATION: ${CCL_CONFIGURATION}")
message(STATUS "PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")

if (NOT DEFINED ${CCL_INSTALL_EXAMPLES})
    set(CCL_INSTALL_EXAMPLES "$ENV{PWD}")
endif()

if (${CMAKE_VERSION} VERSION_LESS 3.1)
#cmake version below 3.1 does not support CMAKE_C[XX}_STANDARD flags
#set manually
    set(CXX_COMPILER_FLAGS "-std=gnu++11")
    set(C_COMPILER_FLAGS "-std=gnu99")
endif()

#common release/debug compilation settings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_COMPILER_FLAGS} -Wall -Werror -D_GNU_SOURCE -fvisibility=internal")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${C_COMPILER_FLAGS} -O0 -g -DENABLE_DEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${C_COMPILER_FLAGS} -O3")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${C_COMPILER_FLAGS} -O2 -g")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_COMPILER_FLAGS} -Wall -Werror -D_GNU_SOURCE -fvisibility=internal")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_COMPILER_FLAGS} -O0 -g -DENABLE_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_COMPILER_FLAGS} -O3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${CXX_COMPILER_FLAGS} -O2 -g")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)
include_directories(${PROJECT_SOURCE_DIR}/../../examples/include)
include_directories(${I_MPI_ROOT}/include/)
include_directories(${CCL_ROOT}/include/${CCL_CONFIGURATION})
link_directories(${I_MPI_ROOT}/lib/release_mt)
link_directories(${I_MPI_ROOT}/libfabric/lib)
link_directories(${CCL_ROOT}/lib/${CCL_CONFIGURATION})

if (COMPUTE_RUNTIME)
    activate_compute_runtime("${CCL_ROOT}/lib;${PROJECT_SOURCE_DIR}/../../cmake" ${COMPUTE_RUNTIME})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPUTE_RUNTIME_FLAGS}")
    list(APPEND CCL_INC_DIRS ${COMPUTE_RUNTIME_INCLUDE_DIRS})
    list(APPEND EXTERNAL_LIBS ${COMPUTE_RUNTIME_LIBRARIES})

    set(MULTI_GPU_SUPPORT ON)
    message ("Enable multi GPU support: ${MULTI_GPU_SUPPORT}")
    message ("COMPUTE_RUNTIME_TARGET_NAME: ${COMPUTE_RUNTIME_TARGET_NAME}")

    if ((${COMPUTE_RUNTIME_TARGET_NAME} STREQUAL "Intel::SYCL"))
        add_subdirectory(sycl)
    endif()
    if(${COMPUTE_RUNTIME_TARGET_NAME} STREQUAL "ze_loader")
        add_subdirectory(l0)
    endif()
endif()

#include other CMakeLists
add_subdirectory(cpu)
#add_subdirectory(resizable)
