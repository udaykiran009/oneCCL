#builds ccl

add_subdirectory(atl)

set(CCL_SRC
    ccl.cpp
    ccl_cpp_api.cpp
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

set(CCL_INC_DIRS
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

#special library that holds objects only
add_library(ccl-objects OBJECT ${CCL_SRC})
set_target_properties(ccl-objects PROPERTIES POSITION_INDEPENDENT_CODE 1)
target_compile_definitions(ccl-objects PRIVATE PT_LOCK_SPIN=1)
target_compile_definitions(ccl-objects PRIVATE ATL_TRANSPORT_DL_DIR=\"${CCL_BUILD_DIR}\")
target_include_directories(ccl-objects PRIVATE ${CCL_INC_DIRS})

#shared library
add_library(ccl SHARED $<TARGET_OBJECTS:ccl-objects>)
target_include_directories(ccl PUBLIC ${CCL_INC_DIRS})
target_link_libraries(ccl PUBLIC dl pthread)
set_target_properties(ccl PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CCL_BUILD_DIR})
install(TARGETS ccl LIBRARY DESTINATION ${CCL_INSTALL_LIB})

#static library
add_library(ccl-static STATIC $<TARGET_OBJECTS:ccl-objects>)
set_target_properties(ccl-static PROPERTIES OUTPUT_NAME ccl)
set_target_properties(ccl-static PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CCL_BUILD_DIR})
install(TARGETS ccl-static ARCHIVE DESTINATION ${CCL_INSTALL_LIB})

#headers installation
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
        DESTINATION ${CCL_INSTALL_INCLUDE})

#mpi & ofi rt
file(GLOB mpi_bins "${PROJECT_SOURCE_DIR}/mpirt/bin/*")
install(PROGRAMS ${mpi_bins} DESTINATION ${CCL_INSTALL_BIN})

install(DIRECTORY ${PROJECT_SOURCE_DIR}/ofi_rt/lib/
        DESTINATION ${CCL_INSTALL_LIB})
