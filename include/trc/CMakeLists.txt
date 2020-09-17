target_sources(
    torch
    PUBLIC
        Animation.h
        AnimationEngine.h
        AssetIds.h
        AssetUtils.h
        AssetRegistry.h
        AssetRegistry.inl
        Boilerplate.h
        CommandCollector.h
        DescriptorProvider.h
        Drawable.h
        DrawableInstanced.h
        Geometry.h
        Light.h
        Material.h
        Node.h
        Particle.h
        PickableRegistry.h
        Pipeline.h
        PipelineBuilder.h
        PipelineDefinitions.h
        Renderer.h
        RenderPass.h
        RenderPassShadow.h
        RenderStage.h
        RenderStageDeferred.h
        Rig.h
        Scene.h
        Vertex.h
)

add_subdirectory(base)
add_subdirectory(utils)
add_subdirectory(data_utils)

target_include_directories(torch PUBLIC ${CMAKE_CURRENT_LIST_DIR})
