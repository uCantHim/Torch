cmake_minimum_required(VERSION 3.14)
project(TorchPipelineCompiler)

option(PIPELINE_COMPILER_ENABLE_SPIRV_FEATURES "" ON)
option(PIPELINE_COMPILER_BUILD_TESTS "" OFF)

# Third-party
include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse
    GIT_TAG        master
)
FetchContent_MakeAvailable(argparse)
find_package(nlohmann_json)
if (NOT nlohmann_json_FOUND)
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json
        GIT_TAG        master
    )

    set(JSON_BuildTests OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(json)
endif ()

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
    src/ShaderDatabase.cpp
    src/TorchCppWriter.cpp
    src/TypeChecker.cpp
    src/TypeConfiguration.cpp
    src/TypeParser.cpp
    src/UniqueName.cpp
    src/VariantFlagSet.cpp
    src/VariantResolver.cpp
    src/shader_tools/ShaderDocument.cpp
    src/shader_tools/ShaderDocumentParser.cpp
)

target_compile_features(pipeline_compiler_lib PRIVATE cxx_std_20)
torch_default_compile_options(pipeline_compiler_lib)

target_include_directories(pipeline_compiler_lib PUBLIC include/)
target_link_libraries(pipeline_compiler_lib
    PRIVATE
        argparse
    PUBLIC
        torch_util
        nlohmann_json::nlohmann_json
)

target_compile_definitions(pipeline_compiler_lib PRIVATE
    STDLIB_DIR="${CMAKE_CURRENT_LIST_DIR}/stdlib/"
    FLAG_COMBINATION_HEADER="${CMAKE_CURRENT_LIST_DIR}/auxiliary/FlagCombination.h"
    UTILS_INCLUDE_FILE="${CMAKE_CURRENT_LIST_DIR}/auxiliary/PipelineCompilerUtils.cpp"
)

if (${PIPELINE_COMPILER_ENABLE_SPIRV_FEATURES})
    target_link_libraries(pipeline_compiler_lib PRIVATE spirv_compiler)
    target_compile_definitions(pipeline_compiler_lib PRIVATE HAS_SPIRV_COMPILER)
endif ()

if (${PIPELINE_COMPILER_BUILD_TESTS})
    add_subdirectory(test)
endif ()

# The actual pipeline compiler executable
add_executable(pipeline_compiler main.cpp)
torch_default_compile_options(pipeline_compiler)
target_link_libraries(pipeline_compiler PRIVATE pipeline_compiler_lib)
target_include_directories(pipeline_compiler PRIVATE src)
