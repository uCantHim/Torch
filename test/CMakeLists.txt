target_compile_definitions(torch PUBLIC
    TRC_TEST_ASSET_DIR="${CMAKE_CURRENT_LIST_DIR}/assets"
    TRC_TEST_FONT_DIR="${CMAKE_CURRENT_LIST_DIR}/fonts"
)

############
# Unit tests

add_subdirectory(unittest)

#######################
# Complex test programs

if (TORCH_BUILD_EXAMPLES)
    add_executable(VkBaseTest vk_base.cpp)
    target_link_libraries(VkBaseTest PUBLIC torch)

    add_executable(BufferCreatePerformanceTest buffer_create_performance.cpp)
    target_link_libraries(BufferCreatePerformanceTest PUBLIC torch)

    add_executable(BasicSetup basic_setup.cpp)
    target_link_libraries(BasicSetup PUBLIC torch)

    add_executable(Viewports viewports.cpp)
    target_link_libraries(Viewports PUBLIC torch)

    add_executable(TorchTest main.cpp)
    target_link_libraries(TorchTest PUBLIC torch)

    add_executable(Font font.cpp)
    target_link_libraries(Font PUBLIC torch)

    add_executable(BasicRay basic_ray.cpp)
    target_link_libraries(BasicRay PUBLIC torch)

    add_executable(RayTracingIntegration rt_integration.cpp)
    target_link_libraries(RayTracingIntegration PUBLIC torch)

    add_executable(MeshShading mesh_shading.cpp)
    target_link_libraries(MeshShading PUBLIC torch)

    if (${TORCH_INTEGRATE_IMGUI})
        add_executable(ImGui imgui_integration.cpp)
        target_link_libraries(ImGui PUBLIC torch torch-imgui)

        add_executable(Window window_functions.cpp)
        target_link_libraries(Window PUBLIC torch torch-imgui)
    endif ()

    if (${TORCH_INTEGRATE_GUI})
        add_executable(Gui gui.cpp)
        target_link_libraries(Gui PUBLIC torch)
    endif()
endif ()

add_executable(Materials materials.cpp)
target_link_libraries(Materials PUBLIC torch pipeline_compiler_lib)
