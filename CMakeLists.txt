cmake_minimum_required(VERSION 3.13)

if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

project(Torch)

# Options
option(TORCH_BUILD_TEST "Build tests for torch" ON)
option(TORCH_DEBUG "Build with debug flags and -logging" OFF)
option(TORCH_USE_FBX_SDK "Toggle use of the FBX SDK and related features. Requires libxml2!" ON)
option(TORCH_INTEGRATE_IMGUI "Use imgui integration" OFF)
option(TORCH_INTEGRATE_GUI "Build integration of custom GUI library" ON)
option(TORCH_OPTIMIZE_SHADERS "Enable optimization for shader compilation" ON)
option(TORCH_BUILD_RAY_TRACING "Include ray tracing facilities in build" ON)

# Other project settings
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Debug settings
if (${TORCH_DEBUG})
    set(CMAKE_BUILD_TYPE Debug)
else ()
    set(CMAKE_BUILD_TYPE Release)
endif ()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DTRC_DEBUG")

# Required third-party
find_package(Freetype REQUIRED)

# Library target
add_library(torch)

target_include_directories(torch
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/vk_base/include
        ${CMAKE_CURRENT_LIST_DIR}/util_lib/include

    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include/trc
        ${CMAKE_CURRENT_LIST_DIR}/include/trc/core
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include/trc
        ${CMAKE_CURRENT_LIST_DIR}/include/trc/core
)

target_link_libraries(
    torch PUBLIC
    vk_base
    TorchUtilLib
    Freetype::Freetype
)

add_subdirectory(shaders)
add_subdirectory(vk_base)
add_subdirectory(util_lib)
add_subdirectory(src)

# Imgui integration
if (${TORCH_INTEGRATE_IMGUI})
    add_library(imgui)
    target_link_libraries(torch PUBLIC imgui)

    target_include_directories(
        imgui PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/external/imgui
        ${CMAKE_CURRENT_LIST_DIR}/external/imgui/backends
    )
    target_sources(
        imgui PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/external/imgui/imgui.h
        ${CMAKE_CURRENT_LIST_DIR}/external/imgui/imgui.cpp
        ${CMAKE_CURRENT_LIST_DIR}/external/imgui/imgui_draw.cpp
        ${CMAKE_CURRENT_LIST_DIR}/external/imgui/imgui_widgets.cpp

        ${CMAKE_CURRENT_LIST_DIR}/external/imgui/backends/imgui_impl_glfw.h
        ${CMAKE_CURRENT_LIST_DIR}/external/imgui/backends/imgui_impl_glfw.cpp
        ${CMAKE_CURRENT_LIST_DIR}/external/imgui/backends/imgui_impl_vulkan.h
        ${CMAKE_CURRENT_LIST_DIR}/external/imgui/backends/imgui_impl_vulkan.cpp
    )
endif ()

# Custom GUI library
if (${TORCH_INTEGRATE_GUI})
    add_subdirectory(src/ui)
endif ()

# Test
if (${TORCH_BUILD_TEST})
    add_subdirectory(test)
endif ()

# Optional FBX stuff
if (${TORCH_USE_FBX_SDK})
    target_link_libraries(torch PUBLIC fbxsdk xml2)
    target_compile_definitions(torch PUBLIC TRC_USE_FBX_SDK)
endif ()

# Optional assimp stuff
cmake_policy(PUSH)
cmake_policy(SET CMP0012 NEW)
find_package(assimp QUIET)
if (${assimp_FOUND})
    target_link_libraries(torch PUBLIC ${ASSIMP_LIBRARIES}) #assimp::assimp)
    target_compile_definitions(torch PUBLIC TRC_USE_ASSIMP)
endif ()
cmake_policy(POP)

#################
# Compile options
target_compile_definitions(torch PUBLIC
    TRC_SHADER_DIR="${CMAKE_CURRENT_LIST_DIR}/shaders"
    TRC_ASSET_DIR="${CMAKE_CURRENT_LIST_DIR}/assets"
    TRC_FONT_DIR="${CMAKE_CURRENT_LIST_DIR}/fonts"
    $<$<BOOL:${TORCH_BUILD_RAY_TRACING}>:TRC_USE_RAY_TRACING>
)
target_compile_options(torch PUBLIC -fPIC)
target_compile_features(torch PUBLIC cxx_std_20)

if (NOT MSVC)
    target_compile_options(torch PRIVATE -Wall -Wextra -Wpedantic)
else()
    target_compile_options(torch PRIVATE /W4)
endif()
