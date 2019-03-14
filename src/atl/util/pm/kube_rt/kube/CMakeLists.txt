#builds kube

set(KUBE_SRC
    helper.c
    int_list.c
    shift_list.c
    kvs_keeper.c
    kube.c
    listener.c)

set(COMMON_KUBE_INC_DIRS
    ${PROJECT_SOURCE_DIR}/src/atl/util/pm/kube_rt/kube
    ${PROJECT_SOURCE_DIR}/ofi_rt/include)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pthread")

#special library that holds objects only
add_library(kube-objects OBJECT ${KUBE_SRC})
set_target_properties(kube-objects PROPERTIES POSITION_INDEPENDENT_CODE 1)
target_include_directories(kube-objects PUBLIC ${COMMON_KUBE_INC_DIRS})
target_compile_definitions(kube-objects PRIVATE HAVE_UNISTD_H HAVE_STDLIB_H HAVE_STRING_H HAVE_STRINGS_H)

#shared lib
add_library(kube SHARED $<TARGET_OBJECTS:kube-objects>)
target_include_directories(kube PUBLIC ${COMMON_KUBE_INC_DIRS})
set_target_properties(kube PROPERTIES VERSION 1 SOVERSION 1.0)
set_target_properties(kube PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${MLSL_BUILD_DIR})

install(TARGETS kube LIBRARY DESTINATION ${MLSL_INSTALL_LIB})

#static lib
add_library(kube-static STATIC $<TARGET_OBJECTS:kube-objects>)
set_target_properties(kube-static PROPERTIES OUTPUT_NAME libkube)
set_target_properties(kube-static PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${MLSL_BUILD_DIR})
install(TARGETS kube-static ARCHIVE DESTINATION ${MLSL_INSTALL_LIB})
