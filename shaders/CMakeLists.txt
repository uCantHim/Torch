set(SHADER_IN_DIR ${CMAKE_CURRENT_LIST_DIR})
set(SHADER_OUT_DIR ${CMAKE_CURRENT_LIST_DIR}/bin)

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

source_group("Shaders" FILES ${SHADERS_GLSL}) # for IDE visibility

find_program(GLSLC glslc HINTS "${Vulkan_INCLUDE_DIRS}/../Bin")
add_executable(shaders fake.cpp)
set_target_properties(shaders PROPERTIES LINKER_LANGUAGE CXX)

if (${GLSLC-NOTFOUND})
    message(WARNING "Failed to find glslc, will not be able to compile "
    "shaders through CMake. Possible fix: Vulkan SDK (re-)installation.")
else()
    foreach(IN_FILE ${SHADERS_GLSL})
        get_filename_component(IN_BASENAME ${IN_FILE} NAME)
        set(OUT_FILE "${IN_FILE}.spv")

        message("Compiling ${IN_FILE} -> ${OUT_FILE}")

        add_custom_command(
            OUTPUT ${OUT_FILE}
            COMMAND ${GLSLC} --target-spv=spv1.4 ${IN_FILE} -o ${OUT_FILE}
            DEPENDS ${IN_FILE}
            IMPLICIT_DEPENDS CXX ${IN_FILE}
        )
        set_source_files_properties(${OUT_FILE} PROPERTIES GENERATED TRUE)

        target_sources(shaders PRIVATE ${OUT_FILE})
    endforeach()
endif()
