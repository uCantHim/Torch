#pragma once

#include <string>
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


        /////////////////////////////////////
        //  Access to the device registry  //
        /////////////////////////////////////

        auto getDeviceRegistry() -> AssetRegistry&;

        template<AssetBaseType T>
        auto getModule() -> AssetRegistryModule<T>&;

    private:
        using AnyTypedID = std::any;

        template<AssetBaseType T>
        auto _loadAsset(const AssetPath& path) -> TypedAssetID<T>;
        template<AssetBaseType T>
        auto _createAsset(u_ptr<AssetSource<T>> source) -> TypedAssetID<T>;
        auto _createBaseAsset(const AssetMetaData& meta) -> AssetID;

        /** Handles device representations of assets */
        AssetRegistry registry;

        data::IdPool assetIdPool;

        /** Stores high-level management-related metadata for all asset types */
        std::unordered_map<AssetID, AssetMetaData> assetMetaData;

        std::unordered_map<AssetPath, AnyTypedID> pathsToAssets;
    };
} // namespace trc

#include "AssetManager.inl"
