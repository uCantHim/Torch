#pragma once

#include <filesystem>

#include <trc_util/Exception.h>

#include "geometry.pb.h"
#include "texture.pb.h"
#include "material.pb.h"
#include "rig.pb.h"
#include "animation.pb.h"
#include "asset.pb.h"

#include "trc/assets/Animation.h"
#include "trc/assets/Geometry.h"
#include "trc/assets/Material.h"
#include "trc/assets/Rig.h"
#include "trc/assets/Texture.h"

namespace trc
{
    namespace fs = std::filesystem;

    namespace internal
    {
        auto serializeAssetData(const AssetData<Geometry>& data) -> serial::Geometry;
        auto serializeAssetData(const AssetData<Texture>& data)  -> serial::Texture;
        auto serializeAssetData(const AssetData<Material>& data)  -> serial::Material;
        auto serializeAssetData(const AssetData<Rig>& data)  -> serial::Rig;
        auto serializeAssetData(const AssetData<Animation>& data)  -> serial::Animation;

        auto deserializeAssetData(const serial::Geometry& geo) -> GeometryData;
        auto deserializeAssetData(const serial::Texture& tex)  -> TextureData;
        auto deserializeAssetData(const serial::Material& mat) -> MaterialData;
        auto deserializeAssetData(const serial::Rig& rig) -> RigData;
        auto deserializeAssetData(const serial::Animation& anim) -> AnimationData;

        void writeAssetToFile(const fs::path& path, const serial::Asset& msg);
        auto loadAssetFromFile(const fs::path& path) -> serial::Asset;
    }

    template<>
    inline auto loadAssetFromFile<Geometry>(const fs::path& path) -> AssetData<Geometry>
    {
        return internal::deserializeAssetData(internal::loadAssetFromFile(path).geometry());
    }

    template<>
    inline auto loadAssetFromFile<Texture>(const fs::path& path) -> AssetData<Texture>
    {
        return internal::deserializeAssetData(internal::loadAssetFromFile(path).texture());
    }

    template<>
    inline auto loadAssetFromFile<Material>(const fs::path& path) -> AssetData<Material>
    {
        return internal::deserializeAssetData(internal::loadAssetFromFile(path).material());
    }

    template<>
    inline auto loadAssetFromFile<Rig>(const fs::path& path) -> AssetData<Rig>
    {
        return internal::deserializeAssetData(internal::loadAssetFromFile(path).rig());
    }

    template<>
    inline auto loadAssetFromFile<Animation>(const fs::path& path) -> AssetData<Animation>
    {
        return internal::deserializeAssetData(internal::loadAssetFromFile(path).animation());
    }

    /** @brief Write asset data in Torch's internal format to a file */
    void saveAssetToFile(const trc::GeometryData& data, const fs::path& path);
    /** @brief Write asset data in Torch's internal format to a file */
    void saveAssetToFile(const trc::TextureData& data, const fs::path& path);
    /** @brief Write asset data in Torch's internal format to a file */
    void saveAssetToFile(const trc::MaterialData& data, const fs::path& path);
    /** @brief Write asset data in Torch's internal format to a file */
    void saveAssetToFile(const trc::RigData& data, const fs::path& path);
    /** @brief Write asset data in Torch's internal format to a file */
    void saveAssetToFile(const trc::AnimationData& data, const fs::path& path);
} // namespace trc
