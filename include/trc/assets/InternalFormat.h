#pragma once

#include <filesystem>

#include <trc_util/Exception.h>

#include "geometry.pb.h"
#include "texture.pb.h"

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

    auto serializeAssetData(const GeometryData& data)  -> trc::serial::Geometry;
    auto serializeAssetData(const TextureData& data)   -> trc::serial::Texture;

    auto deserializeAssetData(const trc::serial::Geometry& geo) -> GeometryData;
    auto deserializeAssetData(const trc::serial::Texture& tex)  -> TextureData;

    void writeToFile(const fs::path& path, const google::protobuf::Message& msg);
    auto loadGeoFromFile(const fs::path& filePath) -> trc::serial::Geometry;
    auto loadTexFromFile(const fs::path& filePath) -> trc::serial::Texture;
} // namespace trc
