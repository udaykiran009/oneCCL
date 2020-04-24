# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindL0
# ----------
#
# Finds Level-Zero API (L0)
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` target ``Level-Zero::L0``, if
# L0 has been found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables::
#
#   L0_FOUND          - True if Level-Zero API was found
#   L0_INCLUDE_DIRS   - include directories for Level-Zero API
#   L0_LIBRARIES      - link against this library to use Level-Zero API
#   L0_VERSION_STRING - Highest supported Level-Zero API version (eg. 1.2)
#   L0_VERSION_MAJOR  - The major version of the Level-Zero API implementation
#   L0_VERSION_MINOR  - The minor version of the Level-Zero API implementation
#
# The module will also define two cache variables::
#
#   L0_INCLUDE_DIR    - the Level-Zero API include directory
#   L0_LIBRARY        - the path to the Level-Zero API library
#

# Prioritize L0
list(APPEND l0_root_hints
            ${L0ROOT}
            $ENV{L0ROOT})
set(original_cmake_prefix_path ${CMAKE_PREFIX_PATH})
if(l0_root_hints)
    list(INSERT CMAKE_PREFIX_PATH 0 ${l0_root_hints})
endif()

function(_FIND_L0_VERSION)
  include(CheckSymbolExists)
  include(CMakePushCheckState)
  set(CMAKE_REQUIRED_QUIET ${L0_FIND_QUIETLY})

  CMAKE_PUSH_CHECK_STATE()
  foreach(VERSION "2_2" "2_1" "2_0" "1_2" "1_1" "1_0")
    set(CMAKE_REQUIRED_INCLUDES "${L0_INCLUDE_DIR}")

    CHECK_SYMBOL_EXISTS(
      L0_VERSION_${VERSION}
        "${L0_INCLUDE_DIR}/CL/cl.h"
        L0_VERSION_${VERSION})
    endif()

    if(L0_VERSION_${VERSION})
      string(REPLACE "_" "." VERSION "${VERSION}")
      set(L0_VERSION_STRING ${VERSION} PARENT_SCOPE)
      string(REGEX MATCHALL "[0-9]+" version_components "${VERSION}")
      list(GET version_components 0 major_version)
      list(GET version_components 1 minor_version)
      set(L0_VERSION_MAJOR ${major_version} PARENT_SCOPE)
      set(L0_VERSION_MINOR ${minor_version} PARENT_SCOPE)
      break()
    endif()
  endforeach()
  CMAKE_POP_CHECK_STATE()
endfunction()

find_path(L0_INCLUDE_DIR
  NAMES
    CL/cl.h OpenCL/cl.h
  PATHS
    ENV "PROGRAMFILES(X86)"
    ENV INTELOCLSDKROOT
    ENV L0_ROOT
  PATH_SUFFIXES
    include
    OpenCL/common/inc)

_FIND_L0_VERSION()

if(WIN32)
  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    find_library(OpenCL_LIBRARY
      NAMES OpenCL
      PATHS
        ENV "PROGRAMFILES(X86)"
        ENV AMDAPPSDKROOT
        ENV INTELOCLSDKROOT
        ENV CUDA_PATH
        ENV NVSDKCOMPUTE_ROOT
        ENV ATISTREAMSDKROOT
        ENV OCL_ROOT
      PATH_SUFFIXES
        "AMD APP/lib/x86"
        lib/x86
        lib/Win32
        OpenCL/common/lib/Win32)
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    find_library(OpenCL_LIBRARY
      NAMES OpenCL
      PATHS
        ENV "PROGRAMFILES(X86)"
        ENV AMDAPPSDKROOT
        ENV INTELOCLSDKROOT
        ENV CUDA_PATH
        ENV NVSDKCOMPUTE_ROOT
        ENV ATISTREAMSDKROOT
        ENV OCL_ROOT
      PATH_SUFFIXES
        "AMD APP/lib/x86_64"
        lib/x86_64
        lib/x64
        OpenCL/common/lib/x64)
  endif()
else()
  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    find_library(OpenCL_LIBRARY
      NAMES OpenCL
      PATHS
        ENV AMDAPPSDKROOT
        ENV CUDA_PATH
      PATH_SUFFIXES
        lib/x86
        lib)
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    find_library(OpenCL_LIBRARY
      NAMES OpenCL
      PATHS
        ENV AMDAPPSDKROOT
        ENV CUDA_PATH
      PATH_SUFFIXES
        lib/x86_64
        lib/x64
        lib
        lib64)
  endif()
endif()

set(OpenCL_LIBRARIES ${OpenCL_LIBRARY})
set(OpenCL_INCLUDE_DIRS ${OpenCL_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  OpenCL
  FOUND_VAR OpenCL_FOUND
  REQUIRED_VARS OpenCL_LIBRARY OpenCL_INCLUDE_DIR
  VERSION_VAR OpenCL_VERSION_STRING)

mark_as_advanced(
  OpenCL_INCLUDE_DIR
  OpenCL_LIBRARY)

if(OpenCL_FOUND AND NOT TARGET OpenCL::OpenCL)
  if(OpenCL_LIBRARY MATCHES "/([^/]+)\\.framework$")
    add_library(OpenCL::OpenCL INTERFACE IMPORTED)
    set_target_properties(OpenCL::OpenCL PROPERTIES
      INTERFACE_LINK_LIBRARIES "${OpenCL_LIBRARY}")
  else()
    add_library(OpenCL::OpenCL UNKNOWN IMPORTED)
    set_target_properties(OpenCL::OpenCL PROPERTIES
      IMPORTED_LOCATION "${OpenCL_LIBRARY}")
  endif()
  set_target_properties(OpenCL::OpenCL PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OpenCL_INCLUDE_DIRS}")
endif()

# Reverting the CMAKE_PREFIX_PATH to its original state
set(CMAKE_PREFIX_PATH ${original_cmake_prefix_path})

