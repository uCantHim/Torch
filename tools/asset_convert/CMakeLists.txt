if (NOT TARGET argparse)
    message(FATAL_ERROR "argparse should be available as a target from the pipeline compiler.")
endif ()

add_executable(convert-asset asset_convert.cpp)

torch_default_compile_options(convert-asset)
target_link_libraries(convert-asset PRIVATE torch argparse)

set_target_properties(convert-asset PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TORCH_EXECUTABLE_OUTPUT_PATH}
)
