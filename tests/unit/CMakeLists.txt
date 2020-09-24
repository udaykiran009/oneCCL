cmake_minimum_required (VERSION 2.8)

if(MULTI_GPU_SUPPORT)
    add_subdirectory(l0)
endif()

add_subdirectory(api)
