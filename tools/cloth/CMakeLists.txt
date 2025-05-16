if (NOT TARGET argparse)
    message(FATAL_ERROR "argparse should be available as a target from the pipeline compiler.")
endif ()


################################################################################
# Cloth Library
################################################################################

add_library(cloth
    src/builtins.cpp
    src/cloth.cpp
    src/document.cpp
    src/DocumentUtil.cpp
    src/parser.cpp
)
torch_default_compile_options(cloth)
target_link_libraries(cloth
    PUBLIC torch
    PRIVATE pipeline_compiler_lib
)
target_include_directories(cloth PRIVATE include/cloth)
target_include_directories(cloth INTERFACE include)


################################################################################
# Cloth Executable
################################################################################

add_executable(cloth-compile cloth_compile_shader.cpp)
torch_default_compile_options(cloth-compile)
target_link_libraries(cloth-compile PRIVATE cloth argparse)
set_target_properties(cloth-compile PROPERTIES
    OUTPUT_NAME cloth
    RUNTIME_OUTPUT_DIRECTORY ${TORCH_EXECUTABLE_OUTPUT_PATH}
)


################################################################################
# Cloth Language Server
################################################################################

FetchContent_Declare(
    lsp
    GIT_REPOSITORY https://github.com/leon-bckl/lsp-framework.git
    GIT_TAG        ce016753aabc3da4db8d20ad6614bca1409cd313
    PATCH_COMMAND  git apply ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lsp.patch
    UPDATE_DISCONNECTED 1  # Don't apply the patch again after populate.
)
FetchContent_MakeAvailable(lsp)

FetchContent_Declare(
    rapidfuzz
    GIT_REPOSITORY https://github.com/rapidfuzz/rapidfuzz-cpp.git
    GIT_TAG        5d078a96d7b5e0c4c39b0295efd9d9280fa11a66
)
FetchContent_MakeAvailable(rapidfuzz)

add_executable(cloth-language-server language-server/main.cpp)
torch_default_compile_options(cloth-language-server)
target_link_libraries(
    cloth-language-server
    PRIVATE
        cloth
        pipeline_compiler_lib
        lsp
        rapidfuzz::rapidfuzz
)
set_target_properties(cloth-language-server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TORCH_EXECUTABLE_OUTPUT_PATH}
)
