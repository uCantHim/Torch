cmake_minimum_required(VERSION 3.16)

if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

if (WIN32)
    if (POLICY CMP0091)
        cmake_policy(SET CMP0091 NEW)
    endif ()
    if (POLICY CMP0077)
        cmake_policy(SET CMP0077 NEW) # option() honors normal variables
    endif ()
endif (WIN32)

project(Torch)

# Options
option(TORCH_FLIP_Y_AXIS "Build Torch in a configuration that has Vulkan's negative y-axis flipped" ON)
option(TORCH_BUILD_SHARED_LIBRARY "Build torch as a shared library" OFF)
option(TORCH_BUILD_TEST "Build tests for torch" ON)
option(TORCH_BUILD_EXAMPLES "Build examples" ON)
option(TORCH_DEBUG "Build with debug flags and -logging" OFF)
option(TORCH_DEBUG_THROW_ON_VALIDATION_ERROR
       "Throw a std::runtime_error from the Vulkan debug callback if a validation error occurs.
       Log all information but do not throw if set to OFF."
       OFF
)
option(TORCH_USE_FBX_SDK "Toggle use of the FBX SDK and related features. Requires libxml2!" ON)
option(TORCH_INTEGRATE_IMGUI "Use imgui integration" OFF)
option(TORCH_INTEGRATE_GUI "Build integration of custom GUI library" ON)
option(TORCH_OPTIMIZE_SHADERS "Enable optimization for shader compilation" ON)
option(TORCH_BUILD_SCENE_EDITOR "Build a graphical scene editing tool" OFF)

# Torch build variables
set(TORCH_EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(TORCH_CONFIG_DIR ${CMAKE_CURRENT_LIST_DIR}/config)
set(TORCH_GENERATED_DIR ${CMAKE_BINARY_DIR}/generated-sources)
set(TORCH_SHADER_DIR ${CMAKE_CURRENT_LIST_DIR}/shaders)
set(TORCH_SHADER_OUTPUT_DIR ${TORCH_GENERATED_DIR}/shaders)
set(TORCH_INTERNAL_SHADER_DB ${TORCH_GENERATED_DIR}/shader-db.json)

if (${TORCH_DEBUG})
    set(CMAKE_BUILD_TYPE DEBUG)
endif()

# Other project settings
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Debug settings
set(TORCH_DEBUG $<$<CONFIG:Debug>:ON>)

# Third-party
find_package(Threads REQUIRED)
include(cmake/utils.cmake)
include(cmake/find_third_party.cmake) # Finds all required third-party dependencies

# In-project dependencies
add_subdirectory(tools)

# Library target
set(TORCH_LIB_TYPE STATIC)
if (TORCH_BUILD_SHARED_LIBRARY)
    set(TORCH_LIB_TYPE SHARED)
endif ()

add_library(torch ${TORCH_LIB_TYPE})
torch_default_compile_options(torch)

target_include_directories(
    torch
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${TORCH_GENERATED_DIR}/include

    PRIVATE
        ${cimg_SOURCE_DIR}
)

target_link_libraries(
    torch
    PUBLIC
        # third-party
        Threads::Threads
        Vulkan::Vulkan
        glfw
        glm
        Freetype::Freetype
        protobuf::libprotobuf
        nlohmann_json::nlohmann_json

        # first-party
        torch_util
        component
        spirv_compiler

    PRIVATE
        PNG::PNG
        pipeline_compiler_lib  # for ShaderDocument
)

target_compile_definitions(torch
    PUBLIC
        TRC_COMPILE_ROOT_DIR="${CMAKE_CURRENT_LIST_DIR}"
        TRC_SHADER_STORAGE_DIR="${TORCH_SHADER_DIR}"
        TRC_SHADER_BINARY_DIR="${TORCH_SHADER_OUTPUT_DIR}"
        TRC_SHADER_DB="${TORCH_INTERNAL_SHADER_DB}"

        # Torch configuration macros
        $<$<BOOL:${TORCH_FLIP_Y_AXIS}>:TRC_FLIP_Y_PROJECTION>

        # Debug settings
        $<$<BOOL:${TORCH_DEBUG}>:TRC_DEBUG>
        $<$<BOOL:${TORCH_DEBUG_THROW_ON_VALIDATION_ERROR}>:TRC_DEBUG_THROW_ON_VALIDATION_ERROR>
        TRC_DEBUG_VULKAN_LOGS_DIR="vulkan_logs"
)

# Sources
add_subdirectory(src)
add_subdirectory(include/trc) # Precompiled headers

# Pipeline generation
include(cmake/build_tools.cmake)
torch_compile_pipelines_cpp(TARGET torch FILE "Common")
torch_compile_pipelines_cpp(TARGET torch FILE "DrawablePipelines")
torch_compile_pipelines_cpp(TARGET torch FILE "RasterPipelines")
torch_compile_pipelines_cpp(TARGET torch FILE "TextPipelines")
torch_compile_pipelines_cpp(TARGET torch FILE "ParticlePipelines")
torch_compile_pipelines_cpp(TARGET torch FILE "RayShaders")

# Protobuf generation
include(protobuf/CMakeLists.txt)

# Imgui integration
if (${TORCH_INTEGRATE_IMGUI})
    target_compile_definitions(torch PUBLIC TRC_USE_IMGUI)

    add_library(imgui)
    torch_default_compile_options(imgui)
    target_link_libraries(torch PUBLIC imgui)

    target_include_directories(
        imgui PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/external/imgui_dynamic_rendering
        ${CMAKE_CURRENT_LIST_DIR}/external/imgui_dynamic_rendering/backends
    )
    target_sources(
        imgui PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/external/imgui_dynamic_rendering/imgui.cpp
        ${CMAKE_CURRENT_LIST_DIR}/external/imgui_dynamic_rendering/imgui_draw.cpp
        ${CMAKE_CURRENT_LIST_DIR}/external/imgui_dynamic_rendering/imgui_tables.cpp
        ${CMAKE_CURRENT_LIST_DIR}/external/imgui_dynamic_rendering/imgui_widgets.cpp

        ${CMAKE_CURRENT_LIST_DIR}/external/imgui_dynamic_rendering/backends/imgui_impl_glfw.cpp
        ${CMAKE_CURRENT_LIST_DIR}/external/imgui_dynamic_rendering/backends/imgui_impl_vulkan.cpp
    )
endif ()

# Custom GUI library
if (${TORCH_INTEGRATE_GUI})
    torch_compile_pipelines_cpp(TARGET torch FILE "GuiShaders")
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
find_package(assimp QUIET)
if (${assimp_FOUND})
    target_link_libraries(torch PUBLIC ${ASSIMP_LIBRARIES})
    target_compile_definitions(torch PUBLIC TRC_USE_ASSIMP)
endif ()
