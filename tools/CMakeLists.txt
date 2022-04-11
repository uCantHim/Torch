######################
#   Required tools   #
######################

add_subdirectory(componentlib)
add_subdirectory(shader_edit)
add_subdirectory(pipeline_definition_language)
add_subdirectory(pre_build_step)


######################
#   Optional tools   #
######################

if (${TORCH_BUILD_SHADER_COMPILER})
    if (NOT TARGET shaderc)
        FetchContent_Declare(
            shaderc
            GIT_REPOSITORY https://github.com/google/shaderc
            GIT_TAG        657c5ed2ba1714c0430895a274a94d6f2aeeab85
            PATCH_COMMAND  ${FETCHCONTENT_BASE_DIR}/shaderc-src/utils/git-sync-deps
        )
        option(SHADERC_SKIP_TESTS "" ON)
        option(SHADERC_SKIP_EXAMPLES "" ON)
        FetchContent_MakeAvailable(shaderc)
    endif()

    add_executable(shader_compiler shader_compiler.cpp)
    torch_default_compile_options(shader_compiler)
    target_link_libraries(shader_compiler PRIVATE torch_util shader_compiler_lib shaderc)
endif ()

if (${TORCH_BUILD_SCENE_EDITOR})
    if (NOT ${TORCH_INTEGRATE_IMGUI})
        message(FATAL_ERROR "ImGui integration must be enabled to build the scene editor")
    endif ()

    add_subdirectory(scene_editor)
endif ()
