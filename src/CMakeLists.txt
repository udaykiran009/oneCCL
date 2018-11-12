#builds mlsl2

add_subdirectory(atl/ofi)
add_subdirectory(atl/util/pm/pmi_rt/pmi)

set(CMAKE_C_COMPILER "icc")
set(CMAKE_CXX_COMPILER "icpc")

set(MLSL_SRC
    mlsl.c
    atl/atl.c
    coll/coll.c
    coll/allreduce.c
    coll/barrier.c
    coll/bcast.c
    coll/reduce.c
    coll/allgatherv.c
    comp/comp.c
    sched/sched.c
    sched/sched_cache.c
    sched/sched_queue.c
    exec/exec.c
    exec/worker.c
    parallelizer/parallelizer.c
    common/comm/comm.cpp
    common/comm/comm_id.cpp
    common/env/env.c
    common/global/global.c
    common/log/log.c
    common/request/request.c)

set(MLSL_INC_DIRS
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -fvisibility=internal")

#special library that holds objects only
add_library(mlsl2-objects OBJECT ${MLSL_SRC})
set_target_properties(mlsl2-objects PROPERTIES POSITION_INDEPENDENT_CODE 1)
target_compile_definitions(mlsl2-objects PRIVATE PT_LOCK_SPIN=1)
target_compile_definitions(mlsl2-objects PRIVATE ATL_TRANSPORT_DL_DIR=\"${MLSL_BUILD_DIR}\")
target_include_directories(mlsl2-objects PRIVATE ${MLSL_INC_DIRS})

#shared library
add_library(mlsl2 SHARED $<TARGET_OBJECTS:mlsl2-objects>)
target_include_directories(mlsl2 PUBLIC ${MLSL_INC_DIRS})
target_link_libraries(mlsl2 PUBLIC dl pthread)
set_target_properties(mlsl2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${MLSL_BUILD_DIR})
install(TARGETS mlsl2 LIBRARY DESTINATION ${MLSL_INSTALL_LIB})

#static library
add_library(mlsl2-static STATIC $<TARGET_OBJECTS:mlsl2-objects>)
set_target_properties(mlsl2-static PROPERTIES OUTPUT_NAME mlsl2)
set_target_properties(mlsl2-static PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${MLSL_BUILD_DIR})
install(TARGETS mlsl2-static ARCHIVE DESTINATION ${MLSL_INSTALL_LIB})

#headers installation
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
        DESTINATION ${MLSL_INSTALL_INCLUDE}
        FILES_MATCHING PATTERN "*.h")
#mpi & ofi rt
file(GLOB mpi_bins "${PROJECT_SOURCE_DIR}/mpirt/bin/*")
install(PROGRAMS ${mpi_bins} DESTINATION ${MLSL_INSTALL_BIN})

install(DIRECTORY ${PROJECT_SOURCE_DIR}/mpirt/lib/
        DESTINATION ${MLSL_INSTALL_LIB})
install(DIRECTORY ${PROJECT_SOURCE_DIR}/ofi_rt/lib/
        DESTINATION ${MLSL_INSTALL_LIB})
