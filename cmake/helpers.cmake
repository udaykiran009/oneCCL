#different functions, etc

function(check_compiler_version)

    set(GCC_MIN_SUPPORTED "4.8")
    set(ICC_MIN_SUPPORTED "15.0")

    if(${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
        if(${CMAKE_C_COMPILER_VERSION} VERSION_LESS ${GCC_MIN_SUPPORTED})
            message(FATAL_ERROR "gcc min supported version is ${GCC_MIN_SUPPORTED}, current version ${CMAKE_C_COMPILER_VERSION}")
        endif()
    elseif(${CMAKE_C_COMPILER_ID} STREQUAL "Intel")
        if(${CMAKE_C_COMPILER_VERSION} VERSION_LESS ${ICC_MIN_SUPPORTED})
            message(FATAL_ERROR "icc min supported version is ${ICC_MIN_SUPPORTED}, current version ${CMAKE_C_COMPILER_VERSION}")
        endif()
    else()
        message(WARNING "Compilation with ${CMAKE_C_COMPILER_ID} was not tested, no warranty")
    endif()

endfunction(check_compiler_version)
