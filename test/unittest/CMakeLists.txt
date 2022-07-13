add_executable(UnitTests)

target_sources(UnitTests
    PRIVATE
        assets_tests/test_asset_path.cpp
        functional/test_maybe.cpp
        data/test_object_pool.cpp
)

target_link_libraries(UnitTests PRIVATE torch)
link_gtest(UnitTests)

include(GoogleTest)
gtest_discover_tests(UnitTests)
