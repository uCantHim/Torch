target_sources(
    torch
    PRIVATE
        AnimationRegistry.cpp
        AssetManager.cpp
        AssetPath.cpp
        AssetRegistry.cpp
        AssetRegistryModuleStorage.cpp
        GeometryRegistry.cpp
        MaterialRegistry.cpp
        RigRegistry.cpp
        SharedDescriptorSet.cpp
        SimpleMaterial.cpp
        TextureRegistry.cpp
)

add_subdirectory(import)
