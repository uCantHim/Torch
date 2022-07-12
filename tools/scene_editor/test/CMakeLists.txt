add_executable(unittest_scene_editor)
target_sources(unittest_scene_editor PRIVATE
    test_hitbox.cpp
)
target_include_directories(unittest_scene_editor PRIVATE ../src/)
target_link_libraries(unittest_scene_editor PRIVATE scene_editor)

# Find googletest
find_package(GTest)
if (${GTEST_FOUND})
    target_link_libraries(unittest_scene_editor PRIVATE GTest::GTest GTest::Main)
else ()
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

    target_link_libraries(unittest_scene_editor PRIVATE gtest_main)
endif ()

enable_testing()
include(GoogleTest)
gtest_discover_tests(unittest_scene_editor)
