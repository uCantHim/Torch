target_sources(
    torch
    PRIVATE
        Animation.cpp
        AnimationEngine.cpp
        AssetRegistry.cpp
        DeferredRenderConfig.cpp
        DescriptorProvider.cpp
        DescriptorProviderWrapper.cpp
        Framebuffer.cpp
        GBuffer.cpp
        Geometry.cpp
        Light.cpp
        LightRegistry.cpp
        Node.cpp
        PickableRegistry.cpp
        PipelineBuilder.cpp
        PipelineDefinitions.cpp
        RenderPassShadow.cpp
        RenderPassDeferred.cpp
        Rig.cpp
        Scene.cpp
        SceneDescriptor.cpp
        ShadowDescriptor.cpp
        Torch.cpp
        TorchResources.cpp
)

add_subdirectory(core)
add_subdirectory(drawable)
add_subdirectory(particle)
add_subdirectory(asset_import)
add_subdirectory(utils)
add_subdirectory(text)
add_subdirectory(ray_tracing)
add_subdirectory(experimental)

target_include_directories(torch PUBLIC ${CMAKE_CURRENT_LIST_DIR})
