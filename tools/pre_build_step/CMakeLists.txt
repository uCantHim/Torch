function(_torch_compile_pipelines_cpp FILE)
    add_custom_command(
        OUTPUT
            ${TORCH_GENERATED_DIR}/${FILE}.cpp
        COMMAND pipeline_compiler ${TORCH_CONFIG_DIR}/${FILE}.se
            -o ${TORCH_GENERATED_DIR} --spv
            --shader-input ${TORCH_SHADER_DIR} --shader-output ${TORCH_SHADER_OUTPUT_DIR}
        DEPENDS
            pipeline_compiler
            ${TORCH_CONFIG_DIR}/${FILE}.se
        VERBATIM
    )
    target_sources(torch PRIVATE ${TORCH_GENERATED_DIR}/${FILE}.cpp)
endfunction()
