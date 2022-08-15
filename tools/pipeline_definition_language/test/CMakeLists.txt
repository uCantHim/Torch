enable_testing()

add_executable(
    pipeline_compiler_test
    FlagCombinationTest.cpp
    ScannerTest.cpp
    VariantFlagSetTest.cpp
    VariantResolverTest.cpp
    test_shader_edit.cpp
)

target_link_libraries(pipeline_compiler_test PRIVATE
    shader_compiler_lib
    pipeline_compiler_lib
)
link_gtest(pipeline_compiler_test)
target_include_directories(pipeline_compiler_test PRIVATE ../src/)
target_compile_definitions(pipeline_compiler_test PRIVATE
    DATADIR="${CMAKE_CURRENT_LIST_DIR}/datadir"
)

include(GoogleTest)
gtest_discover_tests(pipeline_compiler_test)
