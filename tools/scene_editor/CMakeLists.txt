cmake_minimum_required(VERSION 3.14)
project(TorchSceneEditor)

option(SCENE_EDITOR_BUILD_TESTS "Build unittests for the scene editor" ON)

# Third-party
if (NOT TARGET argparse)
    message(FATAL_ERROR "argparse should be available from the pipeline compiler!")
endif ()
if (NOT TARGET nlohmann_json::nlohmann_json)
    message(FATAL_ERROR "nlohmann-json should be available from the pipeline compiler!")
endif ()

add_library(scene_editor SHARED)
torch_default_compile_options(scene_editor)
target_link_libraries(scene_editor PUBLIC
    torch
    torch-imgui
    component
    argparse
    nlohmann_json::nlohmann_json
)

add_subdirectory(src)

# Compile protobuf sources
protobuf_generate_cpp(
    SCENE_EDITOR_PROTO_SRCS SCENE_EDITOR_PROTO_HDRS
    protobuf/hitbox.proto
)
target_sources(scene_editor PRIVATE ${SCENE_EDITOR_PROTO_SRCS})
target_include_directories(scene_editor PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(scene_editor PRIVATE
    SCENE_EDITOR_BUILTIN_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

# Include tests
if (${SCENE_EDITOR_BUILD_TESTS})
    add_subdirectory(test)
endif ()

# Create the scene editor executable
add_executable(scene_editor_exe main.cpp)
target_link_libraries(scene_editor_exe PRIVATE scene_editor)
target_include_directories(scene_editor_exe PRIVATE src)
