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

function (link_gtest TARGET)
    find_package(GTest)
    if (${GTEST_FOUND})
        target_link_libraries(${TARGET} PRIVATE GTest::GTest GTest::Main)
    else ()
        if (NOT TARGET gtest_main)
            FetchContent_Declare(
                googletest
                GIT_REPOSITORY https://github.com/google/googletest
                GIT_TAG main
            )
            # For Windows: Prevent overriding the parent project's compiler/linker settings
            set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

            set(BUILD_GMOCK FALSE)
            set(INSTALL_GTEST FALSE)
            FetchContent_MakeAvailable(googletest)
        endif ()

        target_link_libraries(${TARGET} PRIVATE gtest_main)
    endif ()
endfunction()
