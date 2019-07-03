#builds iccl

add_subdirectory(atl)

set(ICCL_SRC
    iccl.cpp
    iccl_cpp_api.cpp
    atl/atl.cpp
    coll/coll.cpp
    coll/allreduce.cpp
    coll/allreduce_rma.cpp
    coll/barrier.cpp
    coll/bcast.cpp
    coll/double_tree_ops.cpp
    coll/reduce.cpp
    coll/allgatherv.cpp
    coll/sparse.cpp
    comp/comp.cpp
    sched/sched.cpp
    sched/sched_cache.cpp
    sched/sched_queue.cpp
    sched/entry_factory.cpp
    sched/entry/entry.cpp
    sched/entry/postponed_fields.cpp
    exec/exec.cpp
    exec/worker.cpp
    exec/service_worker.cpp
    fusion/fusion.cpp
    parallelizer/parallelizer.cpp
    out_of_order/ooo_match.cpp
    common/comm/atl_tag.cpp
    common/comm/comm.cpp
    common/datatype/datatype.cpp
    common/env/env.cpp
    common/global/global.cpp
    common/log/log.cpp)

set(ICCL_INC_DIRS
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

#special library that holds objects only
add_library(iccl-objects OBJECT ${ICCL_SRC})
set_target_properties(iccl-objects PROPERTIES POSITION_INDEPENDENT_CODE 1)
target_compile_definitions(iccl-objects PRIVATE PT_LOCK_SPIN=1)
target_compile_definitions(iccl-objects PRIVATE ATL_TRANSPORT_DL_DIR=\"${ICCL_BUILD_DIR}\")
target_include_directories(iccl-objects PRIVATE ${ICCL_INC_DIRS})

#shared library
add_library(iccl SHARED $<TARGET_OBJECTS:iccl-objects>)
target_include_directories(iccl PUBLIC ${ICCL_INC_DIRS})
target_link_libraries(iccl PUBLIC dl pthread)
set_target_properties(iccl PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${ICCL_BUILD_DIR})
install(TARGETS iccl LIBRARY DESTINATION ${ICCL_INSTALL_LIB})

#static library
add_library(iccl-static STATIC $<TARGET_OBJECTS:iccl-objects>)
set_target_properties(iccl-static PROPERTIES OUTPUT_NAME iccl)
set_target_properties(iccl-static PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${ICCL_BUILD_DIR})
install(TARGETS iccl-static ARCHIVE DESTINATION ${ICCL_INSTALL_LIB})

#headers installation
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
        DESTINATION ${ICCL_INSTALL_INCLUDE})

#mpi & ofi rt
file(GLOB mpi_bins "${PROJECT_SOURCE_DIR}/mpirt/bin/*")
install(PROGRAMS ${mpi_bins} DESTINATION ${ICCL_INSTALL_BIN})

install(DIRECTORY ${PROJECT_SOURCE_DIR}/ofi_rt/lib/
        DESTINATION ${ICCL_INSTALL_LIB})
