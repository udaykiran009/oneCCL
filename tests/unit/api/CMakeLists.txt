if(API_UT)
    set(UPSTREAM_SOURCE_DIR "${PROJECT_SOURCE_DIR}/../..")
else()
    set(UPSTREAM_SOURCE_DIR ${PROJECT_SOURCE_DIR})
endif()

include(${UPSTREAM_SOURCE_DIR}/cmake/helpers.cmake)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
if (COMPUTE_RUNTIME)
    activate_compute_runtime("${UPSTREAM_SOURCE_DIR}/cmake" ${COMPUTE_RUNTIME})

    if (${CCL_ENABLE_SYCL_V} STREQUAL 1)
        option (CCL_ENABLE_SYCL "Enable CCL SYCL runtime" ON)
        message(STATUS "Enable CCL SYCL runtime")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPUTE_RUNTIME_FLAGS}")
    add_definitions(${DEFINITIONS})
endif()

 if (COMPUTE_RUNTIME)
     if ((${COMPUTE_RUNTIME_TARGET_NAME} STREQUAL "ze_loader")
              OR (${COMPUTE_RUNTIME_TARGET_NAME} STREQUAL "Intel::SYCL"))
        option(MULTI_GPU_SUPPORT "Enable multi GPU support" ON)
        set(MULTI_GPU_SUPPORT ON)
        activate_compute_runtime("${UPSTREAM_SOURCE_DIR}/cmake" L0)
        message ("Enable multi GPU support: ${MULTI_GPU_SUPPORT}")
        message ("COMPUTE_RUNTIME_TARGET_NAME: ${COMPUTE_RUNTIME_TARGET_NAME}")
    endif()
endif(COMPUTE_RUNTIME)

if (MULTI_GPU_SUPPORT)
message("MULTI_GPU_SUPPORTMULTI_GPU_SUPPORTMULTI_GPU_SUPPORTMULTI_GPU_SUPPORTMULTI_GPU_SUPPORTMULTI_GPU_SUPPORTMULTI_GPU_SUPPORTMULTI_GPU_SUPPORT")
    list( APPEND native_api_dependencies_sources
             "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/base.cpp"
             "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/device.cpp"
             "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/driver.cpp"
             "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/export.cpp"
             "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/context.cpp"
             "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/platform.cpp"
             "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/subdevice.cpp"
             "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/primitives.cpp"
             "${UPSTREAM_SOURCE_DIR}/src/native_device_api/l0/utils.cpp"
             "${UPSTREAM_SOURCE_DIR}/src/native_device_api/interop_utils.cpp")
endif()

if (CCL_ENABLE_SYCL)
    list( APPEND native_api_dependencies_sources
            "${UPSTREAM_SOURCE_DIR}/src/native_device_api/sycl/export.cpp")
endif()

if(CCL_ENABLE_SYCL AND MULTI_GPU_SUPPORT)
    list( APPEND native_api_dependencies_sources
            "${UPSTREAM_SOURCE_DIR}/src/native_device_api/sycl_l0/export.cpp")
endif()

list( APPEND native_api_dependencies_sources "${UPSTREAM_SOURCE_DIR}/src/native_device_api/empty/export.cpp")

if(API_UT)
    set(CCL_MAJOR_VERSION     "0")
    set(CCL_MINOR_VERSION     "0")
    set(CCL_UPDATE_VERSION    "0")
    set(CCL_PRODUCT_STATUS    "epsilon")
    string(TIMESTAMP CCL_PRODUCT_BUILD_DATE "%Y-%m-%dT %H:%M:%SZ")
    set(CCL_PRODUCT_FULL "${CCL_PRODUCT_STATUS}-${CCL_MAJOR_VERSION}.${CCL_MINOR_VERSION}.${CCL_UPDATE_VERSION} ${CCL_PRODUCT_BUILD_DATE} ${VCS_INFO}")
    configure_file(${UPSTREAM_SOURCE_DIR}/include/oneapi/ccl/ccl_config.h.in "${CMAKE_CURRENT_BINARY_DIR}/oneapi/ccl/ccl_config.h")
endif(API_UT)

add_library(native_cl_api_objects OBJECT "${native_api_dependencies_sources}")
message("!!!!!!!!!!!!! ${COMPUTE_RUNTIME_TARGET_NAME}")

target_include_directories(native_cl_api_objects PUBLIC
                            ${CMAKE_CURRENT_BINARY_DIR}
                            ${UPSTREAM_SOURCE_DIR}/include
                            ${UPSTREAM_SOURCE_DIR}/src
                            ${UPSTREAM_SOURCE_DIR}/src/atl)
if(COMPUTE_RUNTIME)
    target_include_directories(native_cl_api_objects PUBLIC
                                            $<TARGET_PROPERTY:${COMPUTE_RUNTIME_TARGET_NAME},INTERFACE_INCLUDE_DIRECTORIES>)
    set_target_properties(native_cl_api_objects PROPERTIES
                                            INTERFACE_INCLUDE_DIRECTORIES
                                            $<TARGET_PROPERTY:${COMPUTE_RUNTIME_TARGET_NAME},INTERFACE_INCLUDE_DIRECTORIES>)
endif(COMPUTE_RUNTIME)


add_subdirectory(coll_attrs)
#add_subdirectory(comm_split_attrs)
add_subdirectory(datatype_attrs)

add_subdirectory(device)
add_subdirectory(device_context)
#add_subdirectory(event)
add_subdirectory(stream)
add_subdirectory(communicator)
