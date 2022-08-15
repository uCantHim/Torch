cmake_minimum_required(VERSION 3.14)
project(TorchPipelineCompiler)

option(PIPELINE_COMPILER_ENABLE_SPIRV_FEATURES "" ON)
option(PIPELINE_COMPILER_BUILD_TESTS "" OFF)

# Third-party
include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse
    GIT_TAG        f5ea927f6b3beae2ac924ffde27f570bb9a7d294
)
FetchContent_MakeAvailable(argparse)

# Compiler executable target
add_library(pipeline_compiler_lib
    src/Compiler.cpp
    src/CMakeDepfileWriter.cpp
    src/ErrorReporter.cpp
    src/FlagTable.cpp
    src/IdentifierTable.cpp
    src/Importer.cpp
    src/ObjectConverter.cpp
    src/Parser.cpp
    src/PipelineDataWriter.cpp
    src/PipelineDefinitionLanguage.cpp
    src/Scanner.cpp
    src/TorchCppWriter.cpp
    src/TypeChecker.cpp
    src/TypeConfiguration.cpp
    src/TypeParser.cpp
    src/UniqueName.cpp
    src/VariantFlagSet.cpp
    src/VariantResolver.cpp
)

target_compile_features(pipeline_compiler_lib PRIVATE cxx_std_20)
target_compile_options(pipeline_compiler_lib PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-O0 -g>
    $<$<NOT:$<CONFIG:Debug>>:-O2>
)

target_include_directories(pipeline_compiler_lib PRIVATE src)
target_link_libraries(pipeline_compiler_lib PRIVATE shader_compiler_lib argparse)

target_compile_definitions(pipeline_compiler_lib PRIVATE
    STDLIB_DIR="${CMAKE_CURRENT_LIST_DIR}/stdlib/"
    FLAG_COMBINATION_HEADER="${CMAKE_CURRENT_LIST_DIR}/auxiliary/FlagCombination.h"
    UTILS_INCLUDE_FILE="${CMAKE_CURRENT_LIST_DIR}/auxiliary/PipelineCompilerUtils.cpp"
)

if (${PIPELINE_COMPILER_ENABLE_SPIRV_FEATURES})
    if (NOT TARGET shaderc)
        FetchContent_Declare(
            shaderc
            GIT_REPOSITORY https://github.com/google/shaderc
            GIT_TAG        main
            PATCH_COMMAND  ${FETCHCONTENT_BASE_DIR}/shaderc-src/utils/git-sync-deps
        )

        set(SHADERC_SKIP_TESTS ON)
        set(SHADERC_SKIP_EXAMPLES ON)
        FetchContent_MakeAvailable(shaderc)
    endif()

    target_link_libraries(pipeline_compiler_lib PRIVATE shaderc)
    target_compile_definitions(pipeline_compiler_lib PRIVATE HAS_SPIRV_COMPILER)

    # OpenMP for parallel shader compilation because I'm lazy
    find_package(OpenMP)
    if (OpenMP_CXX_FOUND)
        message(STATUS "Configuring pipeline compiler with OpenMP enabled.")
        target_link_libraries(pipeline_compiler_lib PRIVATE OpenMP::OpenMP_CXX)
    else ()
        message(STATUS "Configuring pipeline compiler without OpenMP enabled - shaders will be compiled sequentially.")
    endif ()
endif ()

if (${PIPELINE_COMPILER_BUILD_TESTS})
    add_subdirectory(test)
endif ()

# The actual pipeline compiler executable
add_executable(pipeline_compiler main.cpp)
target_link_libraries(pipeline_compiler pipeline_compiler_lib)
target_include_directories(pipeline_compiler PRIVATE src)
