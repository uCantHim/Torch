#pragma once

#include <filesystem>

#include <trc_util/Exception.h>

#include "geometry.pb.h"
#include "texture.pb.h"
#include "material.pb.h"
#include "asset.pb.h"

#include "RawData.h"

namespace trc
{
    namespace fs = std::filesystem;

    class FileInputError : public Exception
    {
    public:
        explicit FileInputError(const fs::path& path);
    };

    class FileOutputError : public Exception
    {
    public:
        explicit FileOutputError(const fs::path& path);
    };

    auto serializeAssetData(const GeometryData& data) -> serial::Geometry;
    auto serializeAssetData(const TextureData& data)  -> serial::Texture;
    auto serializeAssetData(const MaterialData& data)  -> serial::Material;

    auto deserializeAssetData(const serial::Geometry& geo) -> GeometryData;
    auto deserializeAssetData(const serial::Texture& tex)  -> TextureData;
    auto deserializeAssetData(const serial::Material& tex) -> MaterialData;

    void writeAssetToFile(const fs::path& path, const serial::Asset& msg);
    auto loadAssetFromFile(const fs::path& path) -> serial::Asset;
} // namespace trc
