set(SHADER_IN_DIR ${CMAKE_CURRENT_LIST_DIR})

file(GLOB SHADERS_GLSL
    # .
    compositing.comp

    # ray tracing
    ray_tracing/anyhit.rahit
    ray_tracing/blue.rmiss
    ray_tracing/reflect.rchit
    ray_tracing/reflect.rgen
)

find_program(GLSLC glslc HINTS "${TORCH_EXECUTABLE_OUTPUT_PATH}")

# A target that depends on all shaders
add_custom_target(compile_shaders)

if (${GLSLC-NOTFOUND})
    message(WARNING "Failed to find glslc, will not be able to compile "
    "shaders through CMake. Possible fix: Vulkan SDK (re-)installation.")
else()
    message(STATUS "Collecting shaders to compile")

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
