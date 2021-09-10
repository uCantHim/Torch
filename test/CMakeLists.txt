find_package(GTest)
include(GoogleTest)

target_compile_definitions(torch PUBLIC
    TRC_TEST_ASSET_DIR="${CMAKE_CURRENT_LIST_DIR}/assets"
    TRC_TEST_FONT_DIR="${CMAKE_CURRENT_LIST_DIR}/fonts"
)

############
# Unit tests

if (${GTEST_FOUND})
    add_executable(TorchUnitTests)

    target_sources(TorchUnitTests
        PRIVATE
            functional/test_maybe.cpp
            data/test_object_pool.cpp
            data/test_typesafe_id.cpp
    )

    target_link_libraries(TorchUnitTests PRIVATE GTest::GTest GTest::Main torch)
    target_compile_options(TorchUnitTests PRIVATE -O0 -g)

    gtest_discover_tests(TorchUnitTests)
endif ()

#######################
# Complex test programs

add_executable(VkBaseTest vk_base.cpp)
target_link_libraries(VkBaseTest PUBLIC vk_base)

add_executable(BasicSetup basic_setup.cpp)
target_link_libraries(BasicSetup PUBLIC torch)

add_executable(TorchTest main.cpp)
target_link_libraries(TorchTest PUBLIC torch)

add_executable(DrawableChainTest drawable_chain.cpp)
target_link_libraries(DrawableChainTest PUBLIC torch)

add_executable(Font font.cpp)
target_link_libraries(Font PUBLIC torch)

add_executable(RayTracing ray_tracing.cpp)
target_link_libraries(RayTracing PUBLIC torch)

if (${TORCH_INTEGRATE_IMGUI})
    add_executable(ImGui imgui_integration.cpp)
    target_link_libraries(ImGui PUBLIC torch)
endif ()

if (${TORCH_INTEGRATE_GUI})
    add_executable(Gui gui.cpp)
    target_link_libraries(Gui PUBLIC torch)
endif()
