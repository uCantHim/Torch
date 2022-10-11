function (torch_compile_pipelines_cpp)
    cmake_parse_arguments(
        ARG
        ""
        "TARGET;FILE"
        ""
        ${ARGN}
    )
    if (NOT ARG_FILE)
        message(FATAL_ERROR "Function _torch_compile_pipelines_cpp expects argument 'FILE'.")
    endif ()
    if (NOT ARG_TARGET)
        message(FATAL_ERROR "Function _torch_compile_pipelines_cpp expects argument 'TARGET'.")
    endif ()

    if (${TORCH_FLIP_Y_AXIS})
        set(SHADER_MACROS --shader-macro TRC_FLIP_Y_AXIS)
    endif ()

    set(_OUT_DIR "${TORCH_GENERATED_DIR}/include/trc")
    set(_DEPFILE ${TORCH_GENERATED_DIR}/${ARG_FILE}_depfile)
    add_custom_command(
        OUTPUT
            ${_OUT_DIR}/${ARG_FILE}.cpp
        COMMAND pipeline_compiler ${TORCH_CONFIG_DIR}/${ARG_FILE}.se
            -o ${_OUT_DIR} --spv --spv-version 1.5 --spv-target-env vulkan1.2
            --shader-input ${TORCH_SHADER_DIR} --shader-output ${TORCH_SHADER_OUTPUT_DIR}
            --shader-db ${TORCH_INTERNAL_SHADER_DB}
            --shader-db-append
            ${SHADER_MACROS}
        DEPENDS
            pipeline_compiler
            ${TORCH_CONFIG_DIR}/${ARG_FILE}.se
        DEPFILE ${_DEPFILE}
        VERBATIM
    )

    target_sources(${ARG_TARGET} PRIVATE ${_OUT_DIR}/${ARG_FILE}.cpp)
endfunction()
