project(TorchPipelineCompiler)

# Third-party
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse
    GIT_TAG        6ba201e2100d12479b1ee00c4c1e07dd21c13ebf
)
FetchContent_MakeAvailable(argparse)

# Compiler executable target
add_executable(pipeline_compiler main.cpp
    src/Compiler.cpp
    src/ErrorReporter.cpp
    src/FlagTable.cpp
    src/IdentifierTable.cpp
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
    src/VariantResolver.cpp
)

target_include_directories(pipeline_compiler PRIVATE src)
target_link_libraries(pipeline_compiler PRIVATE shader_compiler_lib argparse)

target_compile_definitions(pipeline_compiler PRIVATE
    STDLIB_DIR="${CMAKE_CURRENT_LIST_DIR}/stdlib/"
    FLAG_COMBINATION_HEADER="${CMAKE_CURRENT_LIST_DIR}/auxiliary/FlagCombination.h"
)
