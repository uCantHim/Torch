add_custom_target(trc_shader_dependency)

function(_torch_compile_pipelines_cpp FILE)
    if (${TORCH_FLIP_Y_AXIS})
        set(SHADER_MACROS --shader-macro TRC_FLIP_Y_AXIS)
    endif ()

    add_custom_command(
        OUTPUT
            ${TORCH_GENERATED_DIR}/${FILE}.cpp
        COMMAND pipeline_compiler ${TORCH_CONFIG_DIR}/${FILE}.se
            -o ${TORCH_GENERATED_DIR} --spv
            --shader-input ${TORCH_SHADER_DIR} --shader-output ${TORCH_SHADER_OUTPUT_DIR}
            ${SHADER_MACROS}
        DEPENDS
            pipeline_compiler
            ${TORCH_CONFIG_DIR}/${FILE}.se
        VERBATIM
    )

    target_sources(torch PRIVATE ${_OUT_DIR}/${FILE}.cpp)
endfunction()
