add_executable(unittest_scene_editor)
target_sources(unittest_scene_editor PRIVATE
    test_hitbox.cpp
    test_asset_file_index.cpp
)
target_include_directories(unittest_scene_editor PRIVATE ../src/)
target_link_libraries(unittest_scene_editor PRIVATE scene_editor)

# Find googletest
link_gtest(unittest_scene_editor)

enable_testing()
include(GoogleTest)
gtest_discover_tests(unittest_scene_editor)
