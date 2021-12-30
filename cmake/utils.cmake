function (torch_default_compile_options TARGET)
    target_compile_features(${TARGET} PUBLIC cxx_std_20)
    target_compile_options(${TARGET} PRIVATE -fPIC)
    target_compile_options(${TARGET} PRIVATE
        $<$<CONFIG:Debug>:-O0 -g>
        $<$<NOT:$<CONFIG:Debug>>:-O2>
    )

    if (NOT MSVC)
        target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic)
    else()
        target_compile_options(${TARGET} PRIVATE /W4)
    endif()
endfunction()
