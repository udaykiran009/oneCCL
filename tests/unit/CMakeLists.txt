cmake_minimum_required (VERSION 2.8)

if(MULTI_GPU_SUPPORT)
  if (NOT API_UT AND NOT MDEV_APART_UT)
    add_subdirectory(l0)
  endif()

  if (MDEV_APART_UT)
    add_subdirectory(device_topology)
    return()
  endif()
endif()

add_subdirectory(api)
