######################
#   Required tools   #
######################

set(PIPELINE_COMPILER_ENABLE_SPIRV_FEATURES TRUE)

add_subdirectory(componentlib)
add_subdirectory(shader_edit)
add_subdirectory(pipeline_definition_language)
add_subdirectory(pre_build_step)

torch_default_compile_options(pipeline_compiler)
set_target_properties(pipeline_compiler PROPERTIES
    OUTPUT_NAME compile-pipeline
    RUNTIME_OUTPUT_DIRECTORY ${TORCH_EXECUTABLE_OUTPUT_PATH}
)

set_target_properties(glslc_exe PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TORCH_EXECUTABLE_OUTPUT_PATH}
)


######################
#   Optional tools   #
######################

if (${TORCH_BUILD_SCENE_EDITOR})
    if (NOT ${TORCH_INTEGRATE_IMGUI})
        message(FATAL_ERROR "ImGui integration must be enabled to build the scene editor")
    endif ()

    add_subdirectory(scene_editor)

    set_target_properties(scene_editor PROPERTIES
        OUTPUT_NAME scene_editor
        RUNTIME_OUTPUT_DIRECTORY ${TORCH_EXECUTABLE_OUTPUT_PATH}
    )
endif ()
