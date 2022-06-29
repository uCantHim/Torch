file(GLOB TRC_SHADER_DEPENDENCIES
    "${TORCH_SHADER_DIR}/*.vert"  # vertex shader
    "${TORCH_SHADER_DIR}/*.tesc"  # tesselation control shader
    "${TORCH_SHADER_DIR}/*.tese"  # tesselation evaluation shader
    "${TORCH_SHADER_DIR}/*.geom"  # geometry shader
    "${TORCH_SHADER_DIR}/*.frag"  # fragment shader
    "${TORCH_SHADER_DIR}/*.comp"  # compute shader
    "${TORCH_SHADER_DIR}/*.mesh"  # mesh shader
    "${TORCH_SHADER_DIR}/*.task"  # task shader
    "${TORCH_SHADER_DIR}/*.rgen"  # ray generation shader
    "${TORCH_SHADER_DIR}/*.rint"  # ray intersection shader
    "${TORCH_SHADER_DIR}/*.rahit" # ray any hit shader
    "${TORCH_SHADER_DIR}/*.rchit" # ray closest hit shader
    "${TORCH_SHADER_DIR}/*.rmiss" # ray miss shader
    "${TORCH_SHADER_DIR}/*.rcall" # ray callable shader

    "${TORCH_SHADER_DIR}/**/*.vert"  # vertex shader
    "${TORCH_SHADER_DIR}/**/*.tesc"  # tesselation control shader
    "${TORCH_SHADER_DIR}/**/*.tese"  # tesselation evaluation shader
    "${TORCH_SHADER_DIR}/**/*.geom"  # geometry shader
    "${TORCH_SHADER_DIR}/**/*.frag"  # fragment shader
    "${TORCH_SHADER_DIR}/**/*.comp"  # compute shader
    "${TORCH_SHADER_DIR}/**/*.mesh"  # mesh shader
    "${TORCH_SHADER_DIR}/**/*.task"  # task shader
    "${TORCH_SHADER_DIR}/**/*.rgen"  # ray generation shader
    "${TORCH_SHADER_DIR}/**/*.rint"  # ray intersection shader
    "${TORCH_SHADER_DIR}/**/*.rahit" # ray any hit shader
    "${TORCH_SHADER_DIR}/**/*.rchit" # ray closest hit shader
    "${TORCH_SHADER_DIR}/**/*.rmiss" # ray miss shader
    "${TORCH_SHADER_DIR}/**/*.rcall" # ray callable shader
)

function (_torch_compile_pipelines_cpp)
    cmake_parse_arguments(
        ARG
        ""
        "TARGET;FILE"
        "SHADER_DEPS"
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
    add_custom_command(
        OUTPUT
            ${_OUT_DIR}/${ARG_FILE}.cpp
        COMMAND pipeline_compiler ${TORCH_CONFIG_DIR}/${ARG_FILE}.se
            -o ${_OUT_DIR} --spv --spv-version 1.5 --spv-target-env vulkan1.2
            --shader-input ${TORCH_SHADER_DIR} --shader-output ${TORCH_SHADER_OUTPUT_DIR}
            ${SHADER_MACROS}
        DEPENDS
            pipeline_compiler
            ${TORCH_CONFIG_DIR}/${ARG_FILE}.se
            ${ARG_SHADER_DEPS}
        VERBATIM
    )

    target_sources(${ARG_TARGET} PRIVATE ${_OUT_DIR}/${ARG_FILE}.cpp)
endfunction()
