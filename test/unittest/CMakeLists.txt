add_executable(UnitTests)
target_sources(UnitTests
    PRIVATE
        assets_tests/test_asset_manager.cpp
        assets_tests/test_asset_manager_base.cpp
        assets_tests/test_asset_path.cpp
        assets_tests/test_asset_reference.cpp
        assets_tests/test_asset_registry_module_storage.cpp
        assets_tests/test_asset_storage.cpp
        assets_tests/test_asset_trait_storage.cpp
        assets_tests/test_asset_type.cpp
        assets_tests/test_custom_asset.cpp
        assets_tests/test_device_data_cache.cpp
        test_basic_type.cpp
        test_event_handler.cpp
        test_filesystem_data_storage.cpp
        test_raster_scene_base.cpp
        test_shader_code_typechecker.cpp
        test_shader_loader.cpp
        util_tests/test_external_storage.cpp
        util_tests/test_deferred_insert_vector.cpp
        util_tests/test_maybe.cpp
        util_tests/test_memory_stream.cpp
        util_tests/test_object_pool.cpp
        util_tests/test_optional_storage.cpp
        util_tests/test_safe_vector.cpp
        util_tests/test_thread_pool.cpp
        util_tests/test_threadsafe_queue.cpp
)
target_include_directories(UnitTests PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(UnitTests PRIVATE torch)
target_compile_definitions(UnitTests PRIVATE
    DATADIR="${CMAKE_CURRENT_LIST_DIR}/datadir"
    TMPDIR="${CMAKE_CURRENT_BINARY_DIR}/tmpdir"
)
link_gtest(UnitTests)

include(GoogleTest)
gtest_discover_tests(UnitTests)
