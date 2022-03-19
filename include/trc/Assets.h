#pragma once

#include "AssetID.h"

namespace trc
{
    class GeometryRegistry;
    class TextureRegistry;
    class MaterialRegistry;
    struct GeometryData;
    struct TextureData;
    struct MaterialData;

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



    using GeometryID = TypedAssetID<Geometry>;
    using TextureID = TypedAssetID<Texture>;
    using MaterialID = TypedAssetID<Material>;
} // namespace trc
