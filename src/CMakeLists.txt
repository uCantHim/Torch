target_sources(
    torch
    PRIVATE
        Animation.cpp
        AnimationEngine.cpp
        AssetUtils.cpp
        AssetRegistry.cpp
        CommandCollector.cpp
        DescriptorProvider.cpp
        Drawable.cpp
        DrawableInstanced.cpp
        Geometry.cpp
        Light.cpp
        Particle.cpp
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
