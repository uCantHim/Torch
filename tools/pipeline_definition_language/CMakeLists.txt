project(TorchPipelineCompiler)

# Third-party
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse
    GIT_TAG        f5ea927f6b3beae2ac924ffde27f570bb9a7d294
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

if (${PIPELINE_COMPILER_ENABLE_SPIRV_FEATURES})
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

    target_link_libraries(pipeline_compiler PRIVATE shaderc)
    target_compile_definitions(pipeline_compiler PRIVATE HAS_SPIRV_COMPILER)

    # OpenMP for parallel shader compilation because I'm lazy
    find_package(OpenMP)
    if (OpenMP_CXX_FOUND)
        target_link_libraries(pipeline_compiler PRIVATE OpenMP::OpenMP_CXX)
    endif ()
endif ()
