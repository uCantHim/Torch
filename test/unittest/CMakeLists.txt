add_executable(UnitTests)

target_sources(UnitTests
    PRIVATE
        functional/test_maybe.cpp
        data/test_object_pool.cpp
)

target_link_libraries(UnitTests PRIVATE gtest_main torch)
target_compile_options(UnitTests PRIVATE -O0 -g)
target_compile_definitions(UnitTests PRIVATE
    DATADIR="${CMAKE_CURRENT_LIST_DIR}/datadir"
)

if (${TORCH_BUILD_SHADER_COMPILER})
    target_sources(UnitTests PRIVATE test_shader_edit.cpp)
    target_link_libraries(UnitTests PRIVATE shader_compiler_lib)
endif ()

gtest_discover_tests(UnitTests)
