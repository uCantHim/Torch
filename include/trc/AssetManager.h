#pragma once

#include <string>
#include <atomic>
#include <any>
#include <filesystem>

#include <trc_util/data/ObjectId.h>

#include "Assets.h"
#include "AssetPath.h"
#include "AssetSource.h"
#include "AssetRegistry.h"
#include "AssetManagerInterface.h"

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
    class AssetManager : public AssetManagerInterface<AssetManager>
    {
    public:
        AssetManager(const Instance& instance, const AssetRegistryCreateInfo& arInfo);


        //////////////////////
        //  Asset creation  //
        //////////////////////

        auto add(const GeometryData& data) -> GeometryID;
        auto add(const TextureData& data) -> TextureID;
        auto add(const MaterialData& data) -> MaterialID;

        /**
         * @brief Create an asset from in-memory data
         *
         * Asset data imported by this method will permanently stay in host
         * memory until the asset is destroyed.
         */
        template<AssetBaseType T>
        auto create(const AssetData<T>& data) -> TypedAssetID<T>;

        /**
         * @brief Create an asset from any asset data source
         */
        template<AssetBaseType T>
        auto create(u_ptr<AssetSource<T>> source) -> TypedAssetID<T>;

        /**
         * @brief Load an asset in Torch's internal format
         */
        template<AssetBaseType T>
        auto load(const AssetPath& path) -> TypedAssetID<T>;

        /**
         * @brief Completely remove an asset from the asset manager
         */
        template<AssetBaseType T>
        void destroyAsset(TypedAssetID<T> id);


        ///////////////
        //  Queries  //
        ///////////////

        bool exists(const AssetPath& path) const;

        template<AssetBaseType T>
        auto getAsset(const AssetPath& path) const -> TypedAssetID<T>;

        template<AssetBaseType T>
        auto getAssetMetaData(TypedAssetID<T> id) const -> const AssetMetaData&;


        /////////////////////////////////////
        //  Access to the device registry  //
        /////////////////////////////////////

        auto getDeviceRegistry() -> AssetRegistry&;

        template<AssetBaseType T>
        auto getModule() -> AssetRegistryModule<T>&;

    private:
        /**
         * This is the asset source passed to the asset registry.
         *
         * It resolves any asset references when loading its data.
         */
        template<AssetBaseType T>
        class InternalAssetSource : public AssetSource<T>
        {
        public:
            InternalAssetSource(AssetManager& man, u_ptr<AssetSource<T>> impl)
                : manager(&man), source(std::move(impl)) {}

            auto load() -> AssetData<T> override
            {
                auto data = source->load();
                manager->resolveReferences<T>(data);

                return data;
            }

        private:
            AssetManager* manager;
            u_ptr<AssetSource<T>> source;
        };

        std::atomic<ui32> uniqueNameIndex{ 0 };
        auto generateUniqueName() -> std::string;

        template<AssetBaseType T>
        void resolveReferences(AssetData<T>& data);

        template<AssetBaseType T>
        auto _loadAsset(const AssetPath& path) -> TypedAssetID<T>;
        template<AssetBaseType T>
        auto _createAsset(u_ptr<AssetSource<T>> source, AssetMetaData meta) -> TypedAssetID<T>;
        auto _createBaseAsset(AssetMetaData meta) -> AssetID;

        /** Handles device representations of assets */
        AssetRegistry registry;

        data::IdPool assetIdPool;

        /** Stores high-level management-related metadata for all asset types */
        std::unordered_map<AssetID, AssetMetaData> assetMetaData;

        using AnyTypedID = std::any;
        std::unordered_map<AssetPath, AnyTypedID> pathsToAssets;
    };
} // namespace trc

#include "AssetManager.inl"
