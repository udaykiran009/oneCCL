file(GLOB sources "*.c" "*.cpp")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} ${src})
    target_include_directories(${executable} PRIVATE ${EXAMPLES_INC_DIRS})
    target_link_libraries(${executable} PUBLIC rt)
    target_link_libraries(${executable} PUBLIC m)
    target_link_libraries(${executable} PRIVATE ccl)
    # w/a for ats with 2 mpi lib, should be fixed
    target_link_libraries(${executable} -L${I_MPI_ROOT}/lib/release_mt/)
    target_link_libraries(${executable} PUBLIC mpi)
    target_link_libraries(${executable} PRIVATE ${COMPUTE_BACKEND_TARGET_NAME})
    install(TARGETS ${executable} RUNTIME DESTINATION ${CCL_INSTALL_EXAMPLES}/sycl OPTIONAL)
endforeach()
