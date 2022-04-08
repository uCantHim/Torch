#pragma once

#include <filesystem>

#include <trc_util/Exception.h>

#include "geometry.pb.h"
#include "texture.pb.h"
#include "material.pb.h"
#include "rig.pb.h"
#include "animation.pb.h"
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
    auto serializeAssetData(const RigData& data)  -> serial::Rig;
    auto serializeAssetData(const AnimationData& data)  -> serial::Animation;

    auto deserializeAssetData(const serial::Geometry& geo) -> GeometryData;
    auto deserializeAssetData(const serial::Texture& tex)  -> TextureData;
    auto deserializeAssetData(const serial::Material& mat) -> MaterialData;
    auto deserializeAssetData(const serial::Rig& rig) -> RigData;
    auto deserializeAssetData(const serial::Animation& anim) -> AnimationData;

    void writeAssetToFile(const fs::path& path, const serial::Asset& msg);
    auto loadAssetFromFile(const fs::path& path) -> serial::Asset;

    template<>
    inline auto loadAssetData<Geometry>(const fs::path& path) -> AssetData<Geometry>
    {
        return deserializeAssetData(loadAssetFromFile(path).geometry());
    }

    template<>
    inline auto loadAssetData<Texture>(const fs::path& path) -> AssetData<Texture>
    {
        return deserializeAssetData(loadAssetFromFile(path).texture());
    }

    template<>
    inline auto loadAssetData<Material>(const fs::path& path) -> AssetData<Material>
    {
        return deserializeAssetData(loadAssetFromFile(path).material());
    }

    template<>
    inline auto loadAssetData<Rig>(const fs::path& path) -> AssetData<Rig>
    {
        return deserializeAssetData(loadAssetFromFile(path).rig());
    }

    template<>
    inline auto loadAssetData<Animation>(const fs::path& path) -> AssetData<Animation>
    {
        return deserializeAssetData(loadAssetFromFile(path).animation());
    }

    /** @brief Write asset data in Torch's internal format to a file */
    void saveToFile(const trc::GeometryData& data, const fs::path& path);
    /** @brief Write asset data in Torch's internal format to a file */
    void saveToFile(const trc::TextureData& data, const fs::path& path);
    /** @brief Write asset data in Torch's internal format to a file */
    void saveToFile(const trc::MaterialData& data, const fs::path& path);
    /** @brief Write asset data in Torch's internal format to a file */
    void saveToFile(const trc::RigData& data, const fs::path& path);
    /** @brief Write asset data in Torch's internal format to a file */
    void saveToFile(const trc::AnimationData& data, const fs::path& path);
} // namespace trc
