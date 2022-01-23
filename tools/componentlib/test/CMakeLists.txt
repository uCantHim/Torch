find_package(GTest)
include(GoogleTest)

if (${GTEST_FOUND})
    add_executable(unittest_componentlib)

    target_sources(unittest_componentlib PRIVATE
        test_table.cpp
        test_component_storage.cpp
    )

    target_link_libraries(unittest_componentlib PRIVATE GTest::GTest GTest::Main component)

    gtest_discover_tests(unittest_componentlib)
endif ()
