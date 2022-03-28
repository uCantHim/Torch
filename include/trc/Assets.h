#pragma once

#include "AssetID.h"

namespace trc
{
    class GeometryRegistry;
    class TextureRegistry;
    class MaterialRegistry;
    class RigRegistry;
    class AnimationRegistry;
    struct GeometryData;
    struct TextureData;
    struct MaterialData;
    struct RigData;
    struct AnimationData;

    struct Geometry
    {
        using Registry = GeometryRegistry;
        using ImportData = GeometryData;
    };

    struct Texture
    {
        using Registry = TextureRegistry;
        using ImportData = TextureData;
    };

    struct Material
    {
        using Registry = MaterialRegistry;
        using ImportData = MaterialData;
    };

    struct Rig
    {
        using Registry = RigRegistry;
        using ImportData = RigData;
    };

    struct Animation
    {
        using Registry = AnimationRegistry;
        using ImportData = AnimationData;
    };

    using GeometryID = TypedAssetID<Geometry>;
    using TextureID = TypedAssetID<Texture>;
    using MaterialID = TypedAssetID<Material>;
    using RigID = TypedAssetID<Rig>;
    using AnimationID = TypedAssetID<Animation>;
} // namespace trc
