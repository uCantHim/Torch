#pragma once

#include <string>
#include <any>
#include <filesystem>

#include <trc_util/data/ObjectId.h>

#include "AssetIds.h"
#include "AssetRegistry.h"
#include "AssetPath.h"

namespace trc
{
    namespace fs = std::filesystem;

    class AssetDataProxy;

    /**
     * @brief Manages assets on a high level
     *
     * Loads and stores assets from/to disk and manages their storage
     * location on disk. Manages access to asset data in RAM.
     *
     * Does not manage device data!
     */
    class AssetManager
    {
    public:
        AssetManager(const Instance& instance, const AssetRegistryCreateInfo& arInfo);

        auto add(const GeometryData& data) -> GeometryID;
        auto add(const TextureData& data) -> TextureID;
        auto add(const MaterialData& data) -> MaterialID;

        template<AssetBaseType T>
        auto createAsset(const AssetData<T>& data) -> TypedAssetID<T>;

        /**
         * @brief Load an asset in Torch's internal format
         */
        template<AssetBaseType T>
        auto loadAsset(AssetPath path) -> TypedAssetID<T>
        {
            return std::any_cast<TypedAssetID<T>>(_loadAsset(std::move(path)));
        }

        template<AssetBaseType T>
        auto getMetadata(TypedAssetID<T> id) -> const AssetMetaData&;
        auto getMetadata(AssetID id) -> const AssetMetaData&;

        /**
         * @brief Completely remove an asset from the asset manager
         */
        template<AssetBaseType T>
        void destroyAsset(TypedAssetID<T> id);

        auto getDeviceRegistry() -> AssetRegistry&;

    private:
        auto _loadAsset(AssetPath path) -> std::any;
        auto _createAsset(const AssetDataProxy& data) -> std::any;
        auto _createBaseAsset(const AssetMetaData& meta) -> AssetID;

        /** Handles device representations of assets */
        AssetRegistry registry;

        data::IdPool assetIdPool;

        /** Stores high-level management-related metadata for all asset types */
        std::unordered_map<AssetID, AssetMetaData> assetMetaData;
    };
} // namespace trc

#include "AssetManager.inl"
