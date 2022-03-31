target_sources(
    torch
    PRIVATE
        AnimationRegistry.cpp
        AssetManager.cpp
        AssetPath.cpp
        AssetRegistry.cpp
        AssetRegistryModuleStorage.cpp
        AssetSource.cpp
        GeometryRegistry.cpp
        MaterialRegistry.cpp
        RigRegistry.cpp
        SharedDescriptorSet.cpp
        TextureRegistry.cpp
)

add_subdirectory(import)
