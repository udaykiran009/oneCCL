#common CMakeList.txt to build MLSL2, ATL, tests

cmake_minimum_required (VERSION 2.8)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

set(PROJECT_NAME "Intel(R) MLSL")
set(PROJECT_FULL_NAME "Intel(R) Machine Learning Scaling Library (Intel(R) MLSL)")

project(${PROJECT_NAME})

include(cmake/helpers.cmake)

check_compiler_version()

#set default build types.
#Available build types are: Debug, Release, RelWithDebInfo and MinSizeRel
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

#show build info
message(STATUS "Installation directory: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C compiler : ${CMAKE_C_COMPILER}")
message(STATUS "CXX compiler : ${CMAKE_CXX_COMPILER}")

#predefined options
option(USE_SECURITY_FLAGS "Use security flags" FALSE)

#installation path variables
include(GNUInstallDirs)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/_install" CACHE PATH "Default install path" FORCE)
endif()

set(MLSL_INSTALL_LIB "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set(MLSL_INSTALL_INCLUDE "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
set(MLSL_INSTALL_DOC "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DOCDIR}")
set(MLSL_INSTALL_BIN "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
set(MLSL_INSTALL_EXAMPLES "${CMAKE_INSTALL_PREFIX}/examples")

set(CMAKE_INSTALL_RPATH "$ORIGIN")

if (${CMAKE_VERSION} VERSION_LESS 3.1)
#cmake version below 3.1 does not support CMAKE_C[XX}_STANDARD flags
#set manually
    set(CXX_COMPILER_FLAGS "-std=c++11")
    set(C_COMPILER_FLAGS "-std=gnu99")
endif()

#common settings of security options
if(USE_SECURITY_FLAGS AND NOT CMAKE_BUILD_TYPE EQUAL "Debug")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_DEBUG} -Wformat -Wformat-security -D_FORTIFY_SOURCE=2 -fstack-protector")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_DEBUG} -Wformat -Wformat-security -D_FORTIFY_SOURCE=2 -fstack-protector")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -z noexecstack -z relro -z now")
endif()

if(CMAKE_C_COMPILER EQUAL "icc" OR CMAKE_CXX_COMPILER EQUAL "icpc")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-intel")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -static-intel")
endif()

#common release/debug compilation settings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_COMPILER_FLAGS} -Wall -Werror -D_GNU_SOURCE")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${C_COMPILER_FLAGS} -O0 -g -DENABLE_DEBUG=1")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${C_COMPILER_FLAGS} -O3")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${C_COMPILER_FLAGS} -O2 -g")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_COMPILER_FLAGS} -Wall -Werror -D_GNU_SOURCE")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_COMPILER_FLAGS} -O0 -g -DENABLE_DEBUG=1")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_COMPILER_FLAGS} -O3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${CXX_COMPILER_FLAGS} -O2 -g")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MLSL_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/src)

#include other CMakeLists
add_subdirectory(src)
add_subdirectory(examples)

#generate & install mlslvars.sh
configure_file(cmake/mlslvars.sh.in ${CMAKE_CURRENT_BINARY_DIR}/mlslvars.sh @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mlslvars.sh DESTINATION ${MLSL_INSTALL_BIN})

#TODO
#add_custom_target(style-check
#    COMMAND ${PROJECT_SOURCE_DIR}/code-format-check.sh
