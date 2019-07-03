#builds iccl_atl_ofi

add_subdirectory(mpi)

add_subdirectory(util/pm/pmi_rt/pmi)
add_subdirectory(util/pm/kube_rt/kube)

set(OFI_SRC
    ofi/atl_ofi.c
    util/pm/pmi_rt/pmi_rt.c
    util/pm/kube_rt/kube_rt.c)

set(COMMON_OFI_INC_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/util/pm
    ${CMAKE_CURRENT_SOURCE_DIR}/util/pm/codec
    ${CMAKE_CURRENT_SOURCE_DIR}/util/pm/pmi_rt
    ${CMAKE_CURRENT_SOURCE_DIR}/util/pm/kube_rt
    ${PROJECT_SOURCE_DIR}/ofi_rt/include)

#special library that holds objects only
add_library(iccl_atl_ofi-objects OBJECT ${OFI_SRC})
set_target_properties(iccl_atl_ofi-objects PROPERTIES POSITION_INDEPENDENT_CODE 1)
target_include_directories(iccl_atl_ofi-objects PRIVATE ${COMMON_OFI_INC_DIRS})
target_include_directories(iccl_atl_ofi-objects PRIVATE $<TARGET_PROPERTY:pmi,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(iccl_atl_ofi-objects PRIVATE $<TARGET_PROPERTY:kube,INTERFACE_INCLUDE_DIRECTORIES>)

#add library search directory
link_directories(${PROJECT_SOURCE_DIR}/ofi_rt/lib)

#shared
add_library(iccl_atl_ofi SHARED $<TARGET_OBJECTS:iccl_atl_ofi-objects>)
target_include_directories(iccl_atl_ofi PRIVATE ${COMMON_OFI_INC_DIRS})

target_link_libraries(iccl_atl_ofi PRIVATE pmi)
target_link_libraries(iccl_atl_ofi PRIVATE kube)
target_link_libraries(iccl_atl_ofi PRIVATE fabric m)
set_target_properties(iccl_atl_ofi PROPERTIES VERSION 1 SOVERSION 1.0)
set_target_properties(iccl_atl_ofi PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${ICCL_BUILD_DIR})
install(TARGETS iccl_atl_ofi LIBRARY DESTINATION ${ICCL_INSTALL_LIB})

#static
add_library(iccl_atl_ofi-static STATIC $<TARGET_OBJECTS:iccl_atl_ofi-objects>)
set_target_properties(iccl_atl_ofi-static PROPERTIES OUTPUT_NAME iccl_atl_ofi)
set_target_properties(iccl_atl_ofi-static PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${ICCL_BUILD_DIR})
install(TARGETS iccl_atl_ofi-static ARCHIVE DESTINATION ${ICCL_INSTALL_LIB})
