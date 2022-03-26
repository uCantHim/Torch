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

    class Geometry
    {
    public:
        using Registry = GeometryRegistry;
        using ImportData = GeometryData;
    };

    class Texture
    {
    public:
        using Registry = TextureRegistry;
        using ImportData = TextureData;
    };

    class Material
    {
    public:
        using Registry = MaterialRegistry;
        using ImportData = MaterialData;
    };

    class _Rig
    {
    public:
        using Registry = RigRegistry;
        using ImportData = RigData;
    };

    class _Animation
    {
    public:
        using Registry = AnimationRegistry;
        using ImportData = AnimationData;
    };

    using GeometryID = TypedAssetID<Geometry>;
    using TextureID = TypedAssetID<Texture>;
    using MaterialID = TypedAssetID<Material>;
    using RigID = TypedAssetID<_Rig>;
    using AnimationID = TypedAssetID<_Animation>;
} // namespace trc
