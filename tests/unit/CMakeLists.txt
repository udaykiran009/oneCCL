cmake_minimum_required (VERSION 2.8)

if(MULTI_GPU_SUPPORT)
    if (NOT API_UT)
        #add_subdirectory(l0)
    endif()
endif()

add_subdirectory(api)
