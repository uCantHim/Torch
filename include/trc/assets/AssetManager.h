#pragma once

#include <any>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

#include <trc_util/data/IdPool.h>
#include <trc_util/data/TypesafeId.h>

#include "trc/Types.h"
#include "trc/assets/AssetBase.h"
#include "trc/assets/AssetManagerBase.h"
#include "trc/assets/AssetPath.h"
#include "trc/assets/AssetSource.h"
#include "trc/assets/AssetStorage.h"
#include "trc/assets/AssetTraits.h"
#include "trc/assets/AssetTypeMap.h"

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
    class AssetManager : public AssetManagerBase
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
         *        where the AssetManager will load asset data.
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

        using AssetManagerBase::create;
        using AssetManagerBase::destroy;

        /**
         * @brief Create an asset from in-memory data
         *
         * Asset data imported by this method will permanently stay in host
         * memory until the asset is destroyed.
         */
        template<AssetBaseType T>
        auto create(const AssetData<T>& data) -> TypedAssetID<T>;

        /**
         * @brief Create an asset from a path
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
         * @brief Create an asset from a path
         *
         * This implicitly-typed version creates an asset with the correct type
         * by looking up the type information in storage. One might think of the
         * typeless interface as 'informing the asset manager that an asset
         * exists at a path'.
         *
         * # Fails
         *
         * This fails if the type has not been registered previously with
         * `registerAssetType`.
         *
         * # Example
         *
         * ```cpp
         * AssetPath myPath("my/cube");
         * GeometryData data = makeCubeGeo();
         *
         * storage.store<Geometry>(myPath, data);
         * assetManager.create(myPath);
         *
         * assert(assetManager.getAs<Geometry>(myPath));
         * ```
         *
         * @return AssetID The typeless ID of the generated asset. If an asset
         *         with the same path already exists when this method is called,
         *         this is the existing asset's ID.
         *         Returns nullopt if no data exists at `path`.
         *
         * @throw if the type specified in the stored metadata is not registered
         *        at the asset manager.
         */
        auto create(const AssetPath& path) -> std::optional<AssetID>;

        /**
         * @brief Completely remove an asset from the asset manager
         */
        void destroy(AssetID id);

        /**
         * @brief Completely remove an asset from the asset manager
         */
        void destroy(const AssetPath& path);


        ///////////////
        //  Queries  //
        ///////////////

        using AssetManagerBase::getAs;
        using AssetManagerBase::getMetadata;

        bool exists(const AssetPath& path) const;

        template<AssetBaseType T>
        auto getAs(const AssetPath& path) const -> TypedAssetID<T>;

        auto getMetadata(const AssetPath& path) const -> const AssetMetadata&;


        //////////////////////////////////
        //  Access to the data storage  //
        //////////////////////////////////

        auto getDataStorage() -> AssetStorage&;


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
        template<AssetBaseType T>
        void registerDefaultTraits(TraitStorage& traits);

        AssetStorage dataStorage;
        TraitStorage assetTraits;

        std::unordered_map<AssetPath, AssetID> pathsToAssets;
    };



    template<AssetBaseType T>
    inline auto AssetManager::create(const AssetData<T>& data) -> TypedAssetID<T>
    {
        return create<T>(std::make_unique<InMemorySource<T>>(data));
    }

    template<AssetBaseType T>
    inline auto AssetManager::create(const AssetPath& path) -> TypedAssetID<T>
    {
        auto it = pathsToAssets.find(path);
        if (it != pathsToAssets.end())
        {
            try {
                return *getAs<T>(it->second);
            }
            catch (const std::bad_optional_access&)
            {
                throw std::invalid_argument(
                    "[In AssetManager::create(const AssetPath&)]: Tried to create asset of type "
                    + AssetType::make<T>().getName() + " at " + path.string() + ", but asset with"
                    " type " + getAssetType(it->second).getName() + " already exists at this path.");
            }
        }

        const auto id = create<T>(dataStorage.loadDeferred<T>(path));
        pathsToAssets.emplace(path, id.getAssetID());

        return id;
    }

    template<AssetBaseType T>
    inline auto AssetManager::getAs(const AssetPath& path) const -> TypedAssetID<T>
    {
        auto it = pathsToAssets.find(path);
        if (it == pathsToAssets.end())
        {
            throw std::runtime_error("[In AssetManager::getAsset]: Asset at path "
                                     + path.string() + " does not exist");
        }

        try {
            return *getAs<T>(it->second);
        }
        catch (const std::bad_optional_access&)
        {
            throw std::runtime_error(
                "[In AssetManager::get(const AssetPath&)]: Requested asset of type "
                + AssetType::make<T>().getName() + ", but Asset at path " + path.string()
                + " has type " + getAssetType(it->second).getName()
            );
        }
    }
} // namespace trc

#include "trc/assets/DefaultTraits.h"

namespace trc
{
    template<AssetBaseType T>
    inline void AssetManager::registerDefaultTraits(TraitStorage& traits)
    {
        const AssetType type = AssetType::make<T>();
        traits.registerTrait<ManagerTraits>(type, std::make_unique<ManagerTraitsImpl<T>>());
    }
} // namespace trc
