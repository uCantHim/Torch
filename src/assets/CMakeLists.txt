target_sources(
    torch
    PRIVATE
        AnimationRegistry.cpp
        AssetManager.cpp
        AssetManagerBase.cpp
        AssetPath.cpp
        AssetRegistryModuleStorage.cpp
        AssetStorage.cpp
        GeometryRegistry.cpp
        MaterialRegistry.cpp
        RigRegistry.cpp
        SharedDescriptorSet.cpp
        SimpleMaterial.cpp
        TextureRegistry.cpp
)

add_subdirectory(import)
