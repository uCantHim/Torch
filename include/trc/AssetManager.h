#pragma once

#include <string>
#include <filesystem>

#include <trc_util/data/ObjectId.h>

#include "AssetIds.h"
#include "AssetRegistry.h"

namespace trc
{
    namespace fs = std::filesystem;

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

        auto addGeometry(const GeometryData& data) -> GeometryID;
        auto addTexture(const TextureData& data) -> TextureID;

        template<AssetBaseType T>
        auto createAsset(const AssetData<T>& data) -> TypedAssetID<T>;
        template<AssetBaseType T>
        void destroyAsset(TypedAssetID<T> id);

        /**
         * We have something like
         *
         *     struct AssetPath { virtual auto load() = 0; };
         *
         *     struct CacheEntry
         *     {
         *         AssetPath path;
         *
         *         void load()
         *         {
         *             auto data = path.load();
         *             // ...
         *         }
         *     };
         *
         *     struct ExternalPath : AssetPath {};
         *     struct InternalPath : AssetPath {};
         */
        auto importGeometry(const fs::path& filePath) -> GeometryID;
        auto importTexture(const fs::path& filePath) -> TextureID;

        auto getDeviceRegistry() -> AssetRegistry&;

    private:
        /** Handles device representations of assets */
        AssetRegistry registry;

        struct MetaData
        {
            fs::path filePath;
        };

        data::IdPool assetIdPool;

        /** Stores high-level management-related metadata for all asset types */
        std::unordered_map<AssetID, MetaData> assetMetaData;
    };
} // namespace trc

#include "AssetManager.inl"
