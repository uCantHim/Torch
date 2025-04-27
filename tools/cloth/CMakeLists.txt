if (NOT TARGET argparse)
    message(FATAL_ERROR "argparse should be available as a target from the pipeline compiler.")
endif ()

add_library(cloth
    src/cloth.cpp
    src/DocumentUtil.cpp
)
torch_default_compile_options(cloth)
target_link_libraries(cloth
    PUBLIC torch
    PRIVATE pipeline_compiler_lib
)
target_include_directories(cloth PRIVATE include/cloth)
target_include_directories(cloth INTERFACE include)

add_executable(cloth-compile cloth_compile_shader.cpp)
torch_default_compile_options(cloth-compile)
target_link_libraries(cloth-compile PRIVATE cloth argparse pipeline_compiler_lib)
set_target_properties(cloth-compile PROPERTIES
    OUTPUT_NAME cloth
    RUNTIME_OUTPUT_DIRECTORY ${TORCH_EXECUTABLE_OUTPUT_PATH}
)
