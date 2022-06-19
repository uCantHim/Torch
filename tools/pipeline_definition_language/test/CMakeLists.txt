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

enable_testing()

add_executable(
    pipeline_compiler_test
    ScannerTest.cpp
    VariantFlagSetTest.cpp
    VariantResolverTest.cpp
    test_shader_edit.cpp
)

target_link_libraries(pipeline_compiler_test PRIVATE
    shader_compiler_lib
    pipeline_compiler_lib
    gtest_main
)
target_include_directories(pipeline_compiler_test PRIVATE ../src/)
target_compile_definitions(pipeline_compiler_test PRIVATE
    DATADIR="${CMAKE_CURRENT_LIST_DIR}/datadir"
)

include(GoogleTest)
gtest_discover_tests(pipeline_compiler_test)
