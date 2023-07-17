function (torch_default_compile_options TARGET)
    target_compile_features(${TARGET} PUBLIC cxx_std_20)

    if (NOT MSVC)
        target_compile_options(${TARGET} PRIVATE -fPIC)
        target_compile_options(${TARGET} PRIVATE
            $<$<CONFIG:Debug>:-O0 -g>
            $<$<NOT:$<CONFIG:Debug>>:-O2>
        )
        target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic)

        # Generate code coverage when using GCC
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${TARGET} PRIVATE $<$<CONFIG:Debug>:-fprofile-arcs -ftest-coverage --coverage>)
            target_link_options(${TARGET} PRIVATE $<$<CONFIG:Debug>:--coverage>)
            target_link_libraries(${TARGET} PRIVATE gcov)
        endif ()
    else()
        target_compile_options(${TARGET} PRIVATE /W4)
        if (POLICY CMP0091)
            cmake_policy(SET CMP0091 NEW)
        endif ()
    endif()
endfunction()

function (link_gtest TARGET)
    find_package(GTest)
    if (${GTEST_FOUND})
        target_link_libraries(${TARGET} PRIVATE GTest::GTest GTest::Main)
    else ()
        if (NOT TARGET GTest::gtest_main OR NOT TARGET GTest::gmock_main)
            FetchContent_Declare(
                googletest
                GIT_REPOSITORY https://github.com/google/googletest
                GIT_TAG main
            )
            # For Windows: Prevent overriding the parent project's compiler/linker settings
            set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

            set(BUILD_GMOCK TRUE)
            set(INSTALL_GTEST FALSE)
            FetchContent_MakeAvailable(googletest)
        endif ()

        target_link_libraries(${TARGET} PRIVATE GTest::gtest_main GTest::gmock_main)
    endif ()
endfunction()
