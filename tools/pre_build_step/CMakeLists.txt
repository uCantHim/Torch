add_executable(generate_files file_generation.cpp)
target_link_libraries(generate_files PRIVATE shader_compiler_lib)
target_compile_definitions(generate_files
    PRIVATE
    CONFIG_DIR="${TORCH_CONFIG_DIR}"
    SHADER_OUT_DIR="${TORCH_SHADER_DIR}"
)

add_custom_command(
    OUTPUT
        ${TORCH_SHADER_DIR}/asset_registry_descriptor.glsl
        ${TORCH_SHADER_DIR}/drawable/deferred.frag.spv
        ${TORCH_GENERATED_DIR}/Pipelines.cpp
    COMMAND pipeline_compiler ${TORCH_CONFIG_DIR}/Pipelines.se
        -o ${TORCH_GENERATED_DIR} --spv
        --shader-input ${TORCH_CONFIG_DIR} --shader-output ${TORCH_SHADER_DIR}
    DEPENDS
        pipeline_compiler
        ${TORCH_CONFIG_DIR}/Pipelines.se
        ${TORCH_CONFIG_DIR}/asset_registry_descriptor.glsl
    VERBATIM
)

add_custom_target(pre_build_step)
target_sources(pre_build_step PRIVATE
    ${TORCH_SHADER_DIR}/asset_registry_descriptor.glsl
    ${TORCH_GENERATED_DIR}/Pipelines.cpp
)
