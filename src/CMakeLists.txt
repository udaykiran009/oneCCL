#builds mlsl2

add_subdirectory(atl)

set(MLSL_SRC
    mlsl.cpp
    mlsl_cpp_api.cpp
    atl/atl.cpp
    coll/coll.cpp
    coll/allreduce.cpp
    coll/allreduce_rma.cpp
    coll/barrier.cpp
    coll/bcast.cpp
    coll/reduce.cpp
    coll/allgatherv.cpp
    coll/sparse.cpp
    comp/comp.cpp
    sched/sched.cpp
    sched/sched_cache.cpp
    sched/sched_queue.cpp
    sched/entry_factory.cpp
    sched/entry_types/entry.cpp
    sched/entry_types/postponed_fields.cpp
    exec/exec.cpp
    exec/worker.cpp
    exec/service_worker.cpp
    fusion/fusion.cpp
    parallelizer/parallelizer.cpp
    out_of_order/ooo_match.cpp
    out_of_order/postponed_sched.cpp
    out_of_order/tensor_comm.cpp
    common/comm/comm.cpp
    common/datatype/datatype.cpp
    common/env/env.cpp
    common/global/global.cpp
    common/log/log.cpp)

set(MLSL_INC_DIRS
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

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
        DESTINATION ${MLSL_INSTALL_INCLUDE})

#mpi & ofi rt
file(GLOB mpi_bins "${PROJECT_SOURCE_DIR}/mpirt/bin/*")
install(PROGRAMS ${mpi_bins} DESTINATION ${MLSL_INSTALL_BIN})

install(DIRECTORY ${PROJECT_SOURCE_DIR}/ofi_rt/lib/
        DESTINATION ${MLSL_INSTALL_LIB})
