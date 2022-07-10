add_executable(scene_editor main.cpp)
torch_default_compile_options(scene_editor)

# Third-party
if (NOT TARGET argparse)
    message(FATAL_ERROR "argparse should be available from the pipeline compiler!")
endif ()

target_link_libraries(scene_editor PRIVATE torch component argparse)


set(SCENE_EDITOR_BUILD_TESTS FALSE)
if (${SCENE_EDITOR_BUILD_TESTS})
    add_subdirectory(test)
endif ()
