set(EXECUTABLES
    allreduce
    reduce
    bcast
    allgatherv
    priority_allreduce)

foreach(executable ${EXECUTABLES})
    set(src_name ${executable}.c)
    add_executable(${executable} ${src_name})

    target_link_libraries(${executable} PUBLIC rt)
    target_link_libraries(${executable} PRIVATE mlsl2)
    install(TARGETS ${executable} RUNTIME DESTINATION ${MLSL_INSTALL_EXAMPLES})
endforeach()
