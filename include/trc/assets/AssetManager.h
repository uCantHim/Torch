#pragma once

#include <any>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

#include <trc_util/data/IdPool.h>

#include "trc/assets/AssetManagerInterface.h"
#include "trc/assets/AssetPath.h"
#include "trc/assets/AssetRegistry.h"
#include "trc/assets/AssetSource.h"
#include "trc/assets/AssetStorage.h"
#include "trc/assets/AssetTraits.h"
#include "trc/assets/AssetTypeMap.h"
#include "trc/assets/DefaultTraits.h"

namespace trc
{
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
        AssetManager(const AssetManager&) = delete;
        AssetManager(AssetManager&&) noexcept = delete;
        AssetManager& operator=(const AssetManager&) = delete;
        AssetManager& operator=(AssetManager&&) noexcept = delete;

        ~AssetManager() = default;

        /**
         * @brief Constructor
         *
         * @param s_ptr<DataStorage> assetDataStorage The storage from
         *        where the AssetManager will load asset data .
         * @param const Instance& instance
         * @param const AssetRegistryCreateInfo& arInfo The AssetManager
         *        automatically creates an AssetRegistry.
         */
        AssetManager(s_ptr<DataStorage> assetDataStorage,
                     const Instance& instance,
                     const AssetRegistryCreateInfo& arInfo);


        //////////////////////
        //  Asset creation  //
        //////////////////////

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
         *
         * Returns the existing ID if the asset already exists in the
         * AssetManager.
         *
         * @throw std::invalid_argument if an asset exists at `path` and is
         *                              not of type `T`.
         */
        template<AssetBaseType T>
        auto create(const AssetPath& path) -> TypedAssetID<T>;

        /**
         * @brief Completely remove an asset from the asset manager
         */
        template<AssetBaseType T>
        void destroy(TypedAssetID<T> id);

        /**
         * @brief Completely remove an asset from the asset manager
         */
        template<AssetBaseType T>
        void destroy(AssetID id);

        /**
         * @brief Completely remove an asset from the asset manager
         */
        void destroy(const AssetPath& path);


        ///////////////
        //  Queries  //
        ///////////////

        bool exists(const AssetPath& path) const;

        template<AssetBaseType T>
        auto get(const AssetPath& path) const -> TypedAssetID<T>;

        template<AssetBaseType T>
        auto getMetaData(TypedAssetID<T> id) const -> const AssetMetadata&;
        auto getMetaData(const AssetPath& path) const -> const AssetMetadata&;


        /////////////////////////////////////
        //  Access to the device registry  //
        /////////////////////////////////////

        auto getDeviceRegistry() -> AssetRegistry&;
        auto getAssetStorage() -> AssetStorage&;

        template<AssetBaseType T>
        auto getModule() -> AssetRegistryModule<T>&;


        ////////////////////
        //  Asset traits  //
        ////////////////////

        template<AssetBaseType Asset, AssetTraitT Trait>
        auto getTrait() -> Trait& {
            return getTrait<Trait>(AssetType::make<Asset>());
        }

        template<AssetTraitT Trait>
        auto getTrait(const AssetType& type) -> Trait& {
            return assetTraits.getTrait<Trait>(type);
        }

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

            auto getMetadata() -> AssetMetadata override {
                return source->getMetadata();
            }

        private:
            AssetManager* manager;
            u_ptr<AssetSource<T>> source;
        };

        template<AssetBaseType T>
        void resolveReferences(AssetData<T>& data);
        template<AssetBaseType T>
        void registerDefaultTraits(TraitStorage& traits);

        template<AssetBaseType T>
        auto _createAsset(u_ptr<AssetSource<T>> source) -> TypedAssetID<T>;
        auto _createBaseAsset(AssetMetadata meta) -> AssetID;

        /** Handles device representations of assets */
        AssetRegistry registry;
        AssetStorage dataStorage;
        TraitStorage assetTraits;

        struct AssetInfo
        {
            AssetID baseId;
            std::any typedId;

            AssetMetadata metadata;
        };

        data::IdPool<ui32> assetIdPool;
        util::SafeVector<AssetInfo> assetInformation;

        std::unordered_map<AssetPath, AssetID> pathsToAssets;
    };
} // namespace trc

#include "trc/assets/AssetManager.inl"
