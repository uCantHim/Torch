if (NOT TARGET argparse)
    message(FATAL_ERROR "argparse should be available as a target from the pipeline compiler.")
endif ()

foreach(ASSET geometry)
    set(TARGET inspect-${ASSET})
    set(SRC inspect_${ASSET})

    add_executable(${TARGET} ${SRC})

    torch_default_compile_options(${TARGET})
    target_link_libraries(${TARGET} PRIVATE torch argparse)

    set_target_properties(${TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TORCH_EXECUTABLE_OUTPUT_PATH}
    )
endforeach()
