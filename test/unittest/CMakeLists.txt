if (NOT TARGET gtest_main)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest
        GIT_TAG main
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    set(BUILD_GMOCK FALSE)
    set(INSTALL_GTEST FALSE)
    FetchContent_MakeAvailable(googletest)
endif ()

add_executable(UnitTests)

target_sources(UnitTests
    PRIVATE
        functional/test_maybe.cpp
        data/test_object_pool.cpp
)

target_link_libraries(UnitTests PRIVATE gtest_main torch)

include(GoogleTest)
gtest_discover_tests(UnitTests)
