add_executable(generate_files file_generation.cpp)
target_link_libraries(generate_files PRIVATE shader_compiler_lib)
target_compile_definitions(generate_files
    PRIVATE
    CONFIG_DIR="${PROJECT_SOURCE_DIR}/config"
    SHADER_OUT_DIR="${PROJECT_SOURCE_DIR}/shaders"
)

add_custom_target(pre_build_step
    COMMAND generate_files
)
