add_executable(UnitTests)

target_sources(UnitTests
    PRIVATE
        assets_tests/test_asset_path.cpp
        assets_tests/test_custom_asset.cpp
        functional/test_maybe.cpp
        data/test_object_pool.cpp
)

target_link_libraries(UnitTests PRIVATE torch)
target_compile_definitions(UnitTests PRIVATE
    DATADIR="${CMAKE_CURRENT_BINARY_DIR}/datadir"
)
link_gtest(UnitTests)

include(GoogleTest)
gtest_discover_tests(UnitTests)
