find_package(GTest REQUIRED)
include(GoogleTest)

target_compile_definitions(torch PUBLIC
    TRC_TEST_ASSET_DIR="${CMAKE_CURRENT_LIST_DIR}/assets"
    TRC_TEST_FONT_DIR="${CMAKE_CURRENT_LIST_DIR}/fonts"
)

############
# Unit tests

add_subdirectory(unittest)

#######################
# Complex test programs

add_executable(VkBaseTest vk_base.cpp)
target_link_libraries(VkBaseTest PUBLIC vk_base)

add_executable(BasicSetup basic_setup.cpp)
target_link_libraries(BasicSetup PUBLIC torch)

add_executable(Viewports viewports.cpp)
target_link_libraries(Viewports PUBLIC torch)

add_executable(TorchTest main.cpp)
target_link_libraries(TorchTest PUBLIC torch)

add_executable(Font font.cpp)
target_link_libraries(Font PUBLIC torch)

add_executable(RayTracing ray_tracing.cpp)
target_link_libraries(RayTracing PUBLIC torch)

# add_executable(RayTracingIntegration rt_integration.cpp)
# target_link_libraries(RayTracingIntegration PUBLIC torch)

if (${TORCH_INTEGRATE_IMGUI})
    add_executable(ImGui imgui_integration.cpp)
    target_link_libraries(ImGui PUBLIC torch)

    add_executable(Window window_functions.cpp)
    target_link_libraries(Window PUBLIC torch)
endif ()

if (${TORCH_INTEGRATE_GUI})
    add_executable(Gui gui.cpp)
    target_link_libraries(Gui PUBLIC torch)
endif()
