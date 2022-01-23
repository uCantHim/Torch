find_package(GTest)
include(GoogleTest)

if (${GTEST_FOUND})
    add_executable(unittest_scene_editor)

    target_sources(unittest_scene_editor PRIVATE)

    target_link_libraries(unittest_scene_editor PRIVATE GTest::GTest GTest::Main)

    gtest_discover_tests(unittest_scene_editor)
endif ()
