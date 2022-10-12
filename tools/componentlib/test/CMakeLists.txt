add_executable(unittest_componentlib)
torch_default_compile_options(unittest_componentlib)

target_sources(unittest_componentlib PRIVATE
    test_table.cpp
    test_component_storage.cpp
)

target_link_libraries(unittest_componentlib PRIVATE component)
link_gtest(unittest_componentlib)

include(GoogleTest)
gtest_discover_tests(unittest_componentlib)
