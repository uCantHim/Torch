cmake_minimum_required(VERSION 3.14)
project(TorchSceneEditor)

option(SCENE_EDITOR_BUILD_TESTS "Build unittests for the scene editor" ON)

# Third-party
if (NOT TARGET argparse)
    message(FATAL_ERROR "argparse should be available from the pipeline compiler!")
endif ()

add_library(scene_editor)
torch_default_compile_options(scene_editor)
target_link_libraries(scene_editor PUBLIC torch component argparse)

add_subdirectory(src)

if (${SCENE_EDITOR_BUILD_TESTS})
    add_subdirectory(test)
endif ()

# Create the actual executable
add_executable(scene_editor_exe main.cpp)
target_link_libraries(scene_editor_exe PRIVATE scene_editor)
target_include_directories(scene_editor_exe PRIVATE src)
