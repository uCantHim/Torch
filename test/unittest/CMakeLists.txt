add_executable(UnitTests)

target_sources(UnitTests
    PRIVATE
        assets_tests/test_asset_path.cpp
        assets_tests/test_asset_reference.cpp
        assets_tests/test_custom_asset.cpp
        core/test_scene_base.cpp
        test_shader_loader.cpp
        util_tests/test_object_pool.cpp
        util_tests/test_maybe.cpp
        util_tests/test_threadsafe_queue.cpp
        util_tests/test_thread_pool.cpp
)

target_link_libraries(UnitTests PRIVATE torch)
target_compile_definitions(UnitTests PRIVATE
    DATADIR="${CMAKE_CURRENT_LIST_DIR}/datadir"
    TMPDIR="${CMAKE_CURRENT_BINARY_DIR}/tmpdir"
)
link_gtest(UnitTests)

include(GoogleTest)
gtest_discover_tests(UnitTests)
