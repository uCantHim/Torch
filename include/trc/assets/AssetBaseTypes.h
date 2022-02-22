#pragma once

#include "AssetID.h"

namespace trc
{
    class GeometryRegistry;
    class TextureRegistry;
    class MaterialRegistry;
    class RigRegistry;
    class AnimationRegistry;

    struct Geometry
    {
        using Registry = GeometryRegistry;
    };

    struct Texture
    {
        using Registry = TextureRegistry;
    };

    struct Material
    {
        using Registry = MaterialRegistry;
    };

    struct Rig
    {
        using Registry = RigRegistry;
    };

    struct Animation
    {
        using Registry = AnimationRegistry;
    };

    using GeometryData = AssetData<Geometry>;
    using TextureData = AssetData<Texture>;
    using MaterialData = AssetData<Material>;
    using RigData = AssetData<Rig>;
    using AnimationData = AssetData<Animation>;

    using GeometryID = TypedAssetID<Geometry>;
    using TextureID = TypedAssetID<Texture>;
    using MaterialID = TypedAssetID<Material>;
    using RigID = TypedAssetID<Rig>;
    using AnimationID = TypedAssetID<Animation>;
} // namespace trc
