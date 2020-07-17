target_sources(
    torch
    PRIVATE
        AssetRegistry.cpp
        Drawable.cpp
        Geometry.cpp
        Light.cpp
        Node.cpp
        Pipeline.cpp
        PipelineBuilder.cpp
        Renderer.cpp
        Renderpass.cpp
        RenderPassDefinitions.cpp
        Scene.cpp
)

add_subdirectory(base)
add_subdirectory(utils)

target_include_directories(torch PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_executable(Test main.cpp)

target_link_libraries(Test PUBLIC vk_base torch)
target_include_directories(Test PUBLIC ../vk_base/include)
