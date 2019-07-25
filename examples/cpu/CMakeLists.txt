file(GLOB sources "*.c" "*.cpp")

foreach(src ${sources})
    get_filename_component(executable ${src} NAME_WE)
    add_executable(${executable} ${src})

    target_link_libraries(${executable} PUBLIC rt)
    target_link_libraries(${executable} PUBLIC m)
    target_link_libraries(${executable} PRIVATE ccl)
    install(TARGETS ${executable} RUNTIME DESTINATION ${CCL_INSTALL_EXAMPLES})
endforeach()
