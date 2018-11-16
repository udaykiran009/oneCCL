#builds mlsl_atl_ofi

set(OFI_SRC_DIR ${PROJECT_SOURCE_DIR}/src/atl/ofi)
set(PMI_RT_SRC_DIR ${PROJECT_SOURCE_DIR}/src/atl/util/pm/pmi_rt)

set(OFI_SRC
    atl_ofi.c
    ${PMI_RT_SRC_DIR}/pmi_rt.c)

set(COMMON_OFI_INC_DIRS
    ${PROJECT_SOURCE_DIR}/ofi_rt/include
    ${PROJECT_SOURCE_DIR}/src/atl
    ${OFI_SRC_DIR}
    ${PROJECT_SOURCE_DIR}/src/atl/util/pm
    ${PMI_RT_SRC_DIR})

#special library that holds objects only
add_library(mlsl_atl_ofi-objects OBJECT ${OFI_SRC})
set_target_properties(mlsl_atl_ofi-objects PROPERTIES POSITION_INDEPENDENT_CODE 1)
target_include_directories(mlsl_atl_ofi-objects PRIVATE ${COMMON_OFI_INC_DIRS})
target_link_libraries(mlsl_atl_ofi-objects PRIVATE pmi)

#add library search directory
#TODO: uncomment line below when ofi_rt will be updated
#link_directories(${PROJECT_SOURCE_DIR}/ofi_rt/lib)

#shared
add_library(mlsl_atl_ofi SHARED $<TARGET_OBJECTS:mlsl_atl_ofi-objects>)
target_include_directories(mlsl_atl_ofi PRIVATE ${COMMON_OFI_INC_DIRS})
#TODO: remove line below when ofi_rt will be updated
target_link_libraries(mlsl_atl_ofi PRIVATE "-L${PROJECT_SOURCE_DIR}/ofi_rt/lib")

target_link_libraries(mlsl_atl_ofi PRIVATE pmi)
target_link_libraries(mlsl_atl_ofi PRIVATE fabric m)
set_target_properties(mlsl_atl_ofi PROPERTIES VERSION 1 SOVERSION 1.0)
set_target_properties(mlsl_atl_ofi PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${MLSL_BUILD_DIR})
install(TARGETS mlsl_atl_ofi LIBRARY DESTINATION ${MLSL_INSTALL_LIB})

#static
add_library(mlsl_atl_ofi-static STATIC $<TARGET_OBJECTS:mlsl_atl_ofi-objects>)
set_target_properties(mlsl_atl_ofi-static PROPERTIES OUTPUT_NAME mlsl_atl_ofi)
set_target_properties(mlsl_atl_ofi-static PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${MLSL_BUILD_DIR})
install(TARGETS mlsl_atl_ofi-static ARCHIVE DESTINATION ${MLSL_INSTALL_LIB})