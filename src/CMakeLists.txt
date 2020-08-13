target_sources(
    torch
    PRIVATE
        Animation.cpp
        AnimationEngine.cpp
        AssetRegistry.cpp
        CommandCollector.cpp
        DescriptorProvider.cpp
        Drawable.cpp
        DrawableInstanced.cpp
        Geometry.cpp
        Light.cpp
        Node.cpp
        PickableRegistry.cpp
        Pipeline.cpp
        PipelineBuilder.cpp
        PipelineDefinitions.cpp
        Renderer.cpp
        RenderPass.cpp
        RenderPassShadow.cpp
        RenderStage.cpp
        RenderStageDeferred.cpp
        Rig.cpp
        Scene.cpp
)

add_subdirectory(base)
add_subdirectory(utils)

target_include_directories(torch PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_executable(Test main.cpp)

target_link_libraries(Test PUBLIC vk_base torch profiler)
target_include_directories(Test PUBLIC ../vk_base/include)
