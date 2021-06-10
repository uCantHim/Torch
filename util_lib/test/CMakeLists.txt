find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(TorchUtilLibTest)

target_sources(TorchUtilLibTest
    PRIVATE
        functional/test_maybe.cpp
        data/test_object_pool.cpp
        data/test_typesafe_id.cpp
)

target_link_libraries(TorchUtilLibTest
    PRIVATE
        GTest::GTest
        GTest::Main
        TorchUtilLib
)

target_compile_options(TorchUtilLibTest PRIVATE -O0 -g)

gtest_discover_tests(TorchUtilLibTest)
