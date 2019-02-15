#builds mlsl_atl_ofi

add_subdirectory(util/pm/pmi_rt/pmi)

set(OFI_SRC
    ofi/atl_ofi.c
    util/pm/pmi_rt/pmi_rt.c)

set(COMMON_OFI_INC_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/util/pm
    ${CMAKE_CURRENT_SOURCE_DIR}/util/pm/pmi_rt
    ${PROJECT_SOURCE_DIR}/ofi_rt/include)

#special library that holds objects only
add_library(mlsl_atl_ofi-objects OBJECT ${OFI_SRC})
set_target_properties(mlsl_atl_ofi-objects PROPERTIES POSITION_INDEPENDENT_CODE 1)
target_include_directories(mlsl_atl_ofi-objects PRIVATE ${COMMON_OFI_INC_DIRS})
target_include_directories(mlsl_atl_ofi-objects PRIVATE $<TARGET_PROPERTY:pmi,INTERFACE_INCLUDE_DIRECTORIES>)

#add library search directory
link_directories(${PROJECT_SOURCE_DIR}/ofi_rt/lib)

#shared
add_library(mlsl_atl_ofi SHARED $<TARGET_OBJECTS:mlsl_atl_ofi-objects>)
target_include_directories(mlsl_atl_ofi PRIVATE ${COMMON_OFI_INC_DIRS})

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
