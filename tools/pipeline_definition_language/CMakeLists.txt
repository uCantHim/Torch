project(TorchPipelineCompiler)

# Third-party
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse
    GIT_TAG        f5ea927f6b3beae2ac924ffde27f570bb9a7d294
)
FetchContent_MakeAvailable(argparse)

# Compiler executable target
add_library(pipeline_compiler_lib
    src/Compiler.cpp
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
    src/VariantResolver.cpp
)

target_compile_features(pipeline_compiler_lib PRIVATE cxx_std_20)

target_include_directories(pipeline_compiler_lib PRIVATE src)
target_link_libraries(pipeline_compiler_lib PRIVATE shader_compiler_lib argparse)

target_compile_definitions(pipeline_compiler_lib PRIVATE
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
        FetchContent_MakeAvailable(shaderc)
    endif()

    option(SHADERC_SKIP_TESTS "" ON)
    option(SHADERC_SKIP_EXAMPLES "" ON)

    target_link_libraries(pipeline_compiler_lib PRIVATE shaderc)
    target_compile_definitions(pipeline_compiler_lib PRIVATE HAS_SPIRV_COMPILER)

    # OpenMP for parallel shader compilation because I'm lazy
    find_package(OpenMP)
    if (OpenMP_CXX_FOUND)
        target_link_libraries(pipeline_compiler_lib PRIVATE OpenMP::OpenMP_CXX)
    endif ()
endif ()

if (${PIPELINE_COMPILER_BUILD_TESTS})
    if (NOT TARGET googletest)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest
            GIT_TAG main
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif ()

    add_subdirectory(test)
endif ()

# The actual pipeline compiler executable
add_executable(pipeline_compiler main.cpp)
target_link_libraries(pipeline_compiler pipeline_compiler_lib)
target_include_directories(pipeline_compiler PRIVATE src)
