find_package(GTest REQUIRED)
include(GoogleTest)

############
# Unit tests

add_executable(tests)
target_sources(tests PRIVATE utils/test_maybe.cpp)
target_compile_options(tests PRIVATE -O0)
target_link_libraries(tests GTest::GTest GTest::Main torch)
gtest_discover_tests(tests)

#######################
# Complex test programs

add_executable(TorchTest main.cpp)
target_link_libraries(TorchTest PUBLIC torch)

add_executable(DrawableChainTest drawable_chain.cpp)
target_link_libraries(DrawableChainTest PUBLIC torch)
