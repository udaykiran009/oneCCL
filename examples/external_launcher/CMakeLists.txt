file(GLOB sources "*.c" "*.cpp")

link_directories(${EXAMPLES_LIB_DIRS})

foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} ${src})
    target_include_directories(${executable} PRIVATE ${EXAMPLES_INC_DIRS})
    target_link_libraries(${executable} PUBLIC rt)
    target_link_libraries(${executable} PUBLIC m)
    target_link_libraries(${executable} PRIVATE ccl)
    target_link_libraries(${executable} PUBLIC -L${I_MPI_ROOT}/lib/release/)
    target_link_libraries(${executable} PUBLIC mpi)
    target_link_libraries(${executable} PUBLIC ${COMPUTE_BACKEND_TARGET_NAME})
    install(TARGETS ${executable} RUNTIME DESTINATION ${CCL_INSTALL_EXAMPLES}/external_launcher OPTIONAL)
endforeach()

configure_file(run.sh ${CCL_INSTALL_EXAMPLES}/external_launcher/run.sh @ONLY)
configure_file(run_binary.sh ${CCL_INSTALL_EXAMPLES}/external_launcher/run_binary.sh @ONLY)
