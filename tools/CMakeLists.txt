if (${TORCH_BUILD_SHADER_COMPILER})
    add_subdirectory(shader_edit)

    add_executable(shader_compiler shader_compiler.cpp)
    torch_default_compile_options(shader_compiler)
    target_link_libraries(shader_compiler PRIVATE torch_util shader_compiler_lib)
endif ()

if (${TORCH_BUILD_SCENE_EDITOR})
    if (NOT ${TORCH_INTEGRATE_IMGUI})
        message(FATAL_ERROR "ImGui integration must be enabled to build the scene editor")
    endif ()

    add_subdirectory(scene_editor)
endif ()
