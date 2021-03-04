set (dependencies_sources
     ${PROJECT_SOURCE_DIR}/src/comp/bf16/bf16.cpp
     ${PROJECT_SOURCE_DIR}/src/comp/bf16/bf16_intrisics.cpp
     ${PROJECT_SOURCE_DIR}/src/comp/fp16/fp16.cpp
     ${PROJECT_SOURCE_DIR}/src/comp/fp16/fp16_intrisics.cpp)

add_subdirectory(single_device)
add_subdirectory(multi_tile)
add_subdirectory(multi_device)