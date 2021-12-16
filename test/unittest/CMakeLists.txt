if (${GTEST_FOUND})
    add_executable(UnitTests)

    target_sources(UnitTests
        PRIVATE
            functional/test_maybe.cpp
            data/test_object_pool.cpp
            test_shader_edit.cpp
    )

    target_link_libraries(UnitTests PRIVATE GTest::GTest GTest::Main torch)
    target_compile_options(UnitTests PRIVATE -O0 -g)
    target_compile_definitions(UnitTests PRIVATE
        DATADIR="${CMAKE_CURRENT_LIST_DIR}/datadir"
    )

    gtest_discover_tests(UnitTests)
endif ()
