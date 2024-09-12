######################
#   Required tools   #
######################

set(COMPONENTLIB_BUILD_TESTS ${TORCH_BUILD_TEST})

add_subdirectory(componentlib)
add_subdirectory(torch_util)
add_subdirectory(spirv_compiler)

# Pipeline compiler
set(PIPELINE_COMPILER_ENABLE_SPIRV_FEATURES TRUE)
set(PIPELINE_COMPILER_BUILD_TESTS ${TORCH_BUILD_TEST})

add_subdirectory(pipeline_definition_language)

torch_default_compile_options(pipeline_compiler_lib)
set_target_properties(pipeline_compiler PROPERTIES
    OUTPUT_NAME compile-pipeline
    RUNTIME_OUTPUT_DIRECTORY ${TORCH_EXECUTABLE_OUTPUT_PATH}
)

set_target_properties(glslc_exe PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TORCH_EXECUTABLE_OUTPUT_PATH}
)

# Other tools
#add_subdirectory(asset_convert)
#set_target_properties(convert-asset PROPERTIES
#    RUNTIME_OUTPUT_DIRECTORY ${TORCH_EXECUTABLE_OUTPUT_PATH}
#)
#add_subdirectory(asset_inspect)


######################
#   Optional tools   #
######################

if (${TORCH_BUILD_SCENE_EDITOR})
    if (NOT ${TORCH_INTEGRATE_IMGUI})
        message(FATAL_ERROR "ImGui integration must be enabled to build the scene editor")
    endif ()

    add_subdirectory(scene_editor)

    set_target_properties(scene_editor_exe PROPERTIES
        OUTPUT_NAME scene_editor
        RUNTIME_OUTPUT_DIRECTORY ${TORCH_EXECUTABLE_OUTPUT_PATH}
    )
endif ()
