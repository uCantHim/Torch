find_package(GTest)
include(GoogleTest)

if (${GTEST_FOUND})
    add_executable(unittest)

    target_sources(unittest PRIVATE
        test_table.cpp
        test_component_storage.cpp
    )

    target_link_libraries(unittest PRIVATE GTest::GTest GTest::Main component)

    gtest_discover_tests(unittest)
endif ()
