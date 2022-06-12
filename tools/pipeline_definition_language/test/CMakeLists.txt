enable_testing()

add_executable(
   pipeline_compiler_test
   ScannerTest.cpp
   VariantFlagSetTest.cpp
   VariantResolverTest.cpp
)

target_link_libraries(pipeline_compiler_test PRIVATE pipeline_compiler_lib gtest_main)
target_include_directories(pipeline_compiler_test PRIVATE ../src/)

include(GoogleTest)
gtest_discover_tests(pipeline_compiler_test)
