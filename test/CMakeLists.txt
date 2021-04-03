find_package(GTest)
include(GoogleTest)

############
# Unit tests

if (${GTEST_FOUND})
    add_executable(tests)
    target_sources(tests PRIVATE utils/test_maybe.cpp)
    target_compile_options(tests PRIVATE -O0)
    target_link_libraries(tests GTest::GTest GTest::Main torch)
    gtest_discover_tests(tests)
endif ()

#######################
# Complex test programs

add_executable(TorchTest main.cpp)
target_link_libraries(TorchTest PUBLIC torch)

add_executable(DrawableChainTest drawable_chain.cpp)
target_link_libraries(DrawableChainTest PUBLIC torch)

add_executable(BasicSetup basic_setup.cpp)
target_link_libraries(BasicSetup PUBLIC torch)

add_executable(Font font.cpp)
target_link_libraries(Font PUBLIC torch)

if (${TORCH_INTEGRATE_IMGUI})
    add_executable(ImGui imgui_integration.cpp)
    target_link_libraries(ImGui PUBLIC torch)
endif ()

if (${TORCH_INTEGRATE_GUI})
    add_executable(Gui gui.cpp)
    target_link_libraries(Gui PUBLIC torch)
endif()

add_executable(RayTracing ray_tracing.cpp)
target_link_libraries(RayTracing PUBLIC torch)
