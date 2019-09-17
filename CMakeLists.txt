#common CMakeList.txt to build CCL, ATL, tests

cmake_minimum_required (VERSION 2.8)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

set(PROJECT_NAME "oneAPI Collective Communications Library")
set(PROJECT_FULL_NAME "oneAPI Collective Communications Library")

project(${PROJECT_NAME})

include(cmake/helpers.cmake)

check_compiler_version()

#set default build types.
#Available build types are: Debug, Release, RelWithDebInfo and MinSizeRel
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

#show build info
message(STATUS "Installation directory: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C compiler : ${CMAKE_C_COMPILER}")
message(STATUS "CXX compiler : ${CMAKE_CXX_COMPILER}")

#predefined options
option(USE_SECURITY_FLAGS "Use security flags" FALSE)
option(USE_CODECOV_FLAGS "Calculate code coverage" FALSE)
option(WITH_ASAN "Use address sanitizer, can only be used in Debug build" FALSE)

#installation path variables
include(GNUInstallDirs)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/_install" CACHE PATH "Default install path" FORCE)
endif()

set(CCL_COMMON_INSTALL_PREFIX "intel64")
set(CMAKE_INSTALL_LIBDIR "lib")
set(CCL_INSTALL_LIB "${CCL_COMMON_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set(CCL_INSTALL_INCLUDE "${CCL_COMMON_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
set(CCL_INSTALL_DOC "${CCL_COMMON_INSTALL_PREFIX}/${CMAKE_INSTALL_DOCDIR}")
set(CCL_INSTALL_BIN "${CCL_COMMON_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
set(CCL_INSTALL_ENV "${CCL_COMMON_INSTALL_PREFIX}/env")
set(CCL_INSTALL_MODULE "${CCL_COMMON_INSTALL_PREFIX}/env/modulefiles")
set(CCL_INSTALL_EXAMPLES "${CMAKE_INSTALL_PREFIX}/examples")
set(CCL_INSTALL_TESTS "${CMAKE_INSTALL_PREFIX}/tests/functional")

set(CMAKE_SKIP_INSTALL_RPATH TRUE)
set(CMAKE_SKIP_RPATH TRUE)

if (${CMAKE_VERSION} VERSION_LESS 3.1)
#cmake version below 3.1 does not support CMAKE_C[XX}_STANDARD flags
#set manually
    set(CXX_COMPILER_FLAGS "-std=gnu++11")
    set(C_COMPILER_FLAGS "-std=gnu99")
endif()

#common settings of security options
if(USE_SECURITY_FLAGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat -Wformat-security -D_FORTIFY_SOURCE=2 -fstack-protector")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat -Wformat-security -D_FORTIFY_SOURCE=2 -fstack-protector")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -z noexecstack -z relro -z now")
endif()

if(${CMAKE_C_COMPILER_ID} STREQUAL "Intel" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
	if (USE_CODECOV_FLAGS)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -prof-gen=srcpos -prof-src-root-cwd")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -prof-gen=srcpos -prof-src-root-cwd")
	endif()
endif()

#TODO: add -Wextra to c/cxx flags

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
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(${CMAKE_C_COMPILER_ID} STREQUAL "Clang" AND ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(CMAKE_CLANG_FLAGS "-Wno-pessimizing-move -Wno-dangling-field -Wno-return-type -Wno-defaulted-function-deleted -Wno-mismatched-tags -Wno-unused-private-field -Wno-unneeded-internal-declaration -Wno-delete-non-abstract-non-virtual-dtor -Wno-reorder -DENABLE_SYCL=1 -D__STRICT_ANSI__ -fsycl")
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

if(WITH_ASAN AND ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message(STATUS "Compiling with address sanitizer")
    set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS_DEBUG  "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lasan")
endif()

set(CCL_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/src)

enable_testing()

set(EXAMPLES_INC_DIRS ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/examples/include)

#include other CMakeLists
add_subdirectory(src)
add_subdirectory(examples/cpu)
if(${CMAKE_C_COMPILER_ID} STREQUAL "Clang" AND ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    add_subdirectory(examples/sycl)
endif()
add_subdirectory(examples/common)
add_subdirectory(tests/functional)


#generate & install cclvars.sh
configure_file(cmake/cclvars.sh.in ${CMAKE_CURRENT_BINARY_DIR}/cclvars.sh @ONLY)
configure_file(cmake/vars.sh.in ${CMAKE_CURRENT_BINARY_DIR}/vars.sh @ONLY)
configure_file(cmake/ccl ${CMAKE_CURRENT_BINARY_DIR}/ccl @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/cclvars.sh DESTINATION ${CCL_INSTALL_BIN})
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/vars.sh DESTINATION ${CCL_INSTALL_ENV})
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/ccl DESTINATION ${CCL_INSTALL_MODULE})

#CPack re-uses generated install rules and puts all targets into archive
#archive creation, generates a target 'package'
set(CPACK_GENERATOR TGZ STGZ)
set(CPACK_PACKAGE_VERSION_MAJOR 2021)
set(CPACK_PACKAGE_VERSION_MINOR 1-beta01)
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
set(CPACK_PACKAGE_NAME "ccl")
set(CPACK_PACKAGE_VENDOR "Intel(R)")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_FULL_NAME}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION 1)

include(CPack)

#wrap cpack call into 'archive' target
add_custom_target(archive
                  COMMAND "${CMAKE_CPACK_COMMAND}")

#TODO
#add_custom_target(style-check
#    COMMAND ${PROJECT_SOURCE_DIR}/code-format-check.sh
