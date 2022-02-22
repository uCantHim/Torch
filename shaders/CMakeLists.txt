set(SHADER_IN_DIR ${CMAKE_CURRENT_LIST_DIR})

file(GLOB SHADERS_GLSL
    "${SHADER_IN_DIR}/*.vert"  # vertex shader
    "${SHADER_IN_DIR}/*.tesc"  # tesselation control shader
    "${SHADER_IN_DIR}/*.tese"  # tesselation evaluation shader
    "${SHADER_IN_DIR}/*.geom"  # geometry shader
    "${SHADER_IN_DIR}/*.frag"  # fragment shader
    "${SHADER_IN_DIR}/*.comp"  # compute shader
    "${SHADER_IN_DIR}/*.mesh"  # mesh shader
    "${SHADER_IN_DIR}/*.task"  # task shader
    "${SHADER_IN_DIR}/*.rgen"  # ray generation shader
    "${SHADER_IN_DIR}/*.rint"  # ray intersection shader
    "${SHADER_IN_DIR}/*.rahit" # ray any hit shader
    "${SHADER_IN_DIR}/*.rchit" # ray closest hit shader
    "${SHADER_IN_DIR}/*.rmiss" # ray miss shader
    "${SHADER_IN_DIR}/*.rcall" # ray callable shader

    "${SHADER_IN_DIR}/**/*.vert"  # vertex shader
    "${SHADER_IN_DIR}/**/*.tesc"  # tesselation control shader
    "${SHADER_IN_DIR}/**/*.tese"  # tesselation evaluation shader
    "${SHADER_IN_DIR}/**/*.geom"  # geometry shader
    "${SHADER_IN_DIR}/**/*.frag"  # fragment shader
    "${SHADER_IN_DIR}/**/*.comp"  # compute shader
    "${SHADER_IN_DIR}/**/*.mesh"  # mesh shader
    "${SHADER_IN_DIR}/**/*.task"  # task shader
    "${SHADER_IN_DIR}/**/*.rgen"  # ray generation shader
    "${SHADER_IN_DIR}/**/*.rint"  # ray intersection shader
    "${SHADER_IN_DIR}/**/*.rahit" # ray any hit shader
    "${SHADER_IN_DIR}/**/*.rchit" # ray closest hit shader
    "${SHADER_IN_DIR}/**/*.rmiss" # ray miss shader
    "${SHADER_IN_DIR}/**/*.rcall" # ray callable shader
)

find_program(GLSLC glslc HINTS "${Vulkan_INCLUDE_DIRS}/../Bin")

# A target that depends on all shaders
add_custom_target(compile_shaders)

if (${GLSLC-NOTFOUND})
    message(WARNING "Failed to find glslc, will not be able to compile "
    "shaders through CMake. Possible fix: Vulkan SDK (re-)installation.")
else()
    message("-- Collecting shaders to compile")

    foreach(IN_FILE ${SHADERS_GLSL})
        set(OUT_FILE "${IN_FILE}.spv")

        if (${TORCH_OPTIMIZE_SHADERS})
            set(SHADER_OPTIMIZATION_FLAG "-O")
        else()
            set(SHADER_OPTIMIZATION_FLAG "-O0")
        endif ()

        add_custom_command(
            OUTPUT ${OUT_FILE}
            COMMAND ${GLSLC} ${SHADER_OPTIMIZATION_FLAG} --target-spv=spv1.4 --target-env=vulkan1.2 $<$<BOOL:TORCH_FLIP_Y_AXIS>:-DTRC_FLIP_Y_AXIS> ${IN_FILE} -o ${OUT_FILE}
            DEPENDS ${IN_FILE}
            IMPLICIT_DEPENDS CXX ${IN_FILE}  # Detect changes in included files
            VERBATIM
        )
        set_source_files_properties(${OUT_FILE} PROPERTIES GENERATED TRUE)

        target_sources(compile_shaders PRIVATE ${OUT_FILE})
    endforeach()
endif()
