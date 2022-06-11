enable_testing()

add_executable(
   pipeline_compiler_test
   ScannerTest.cpp
   VariantResolverTest.cpp
)

target_link_libraries(pipeline_compiler_test PRIVATE pipeline_compiler_lib gtest_main)

include(GoogleTest)
gtest_discover_tests(pipeline_compiler_test)
