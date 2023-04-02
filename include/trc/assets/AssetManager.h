#pragma once

#include <optional>
#include <stdexcept>
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
     * An extension of `AssetManagerBase` that adds automatically-typed
     * interfaces that allow asset lookup and destruction via the typeless
     * `AssetID` without explicit specification of the asset's expected type.
     *
     * If an object exists but some kind of parameters are invalid, we return
     * std::optional.
     * If the object at an ID/path or other kind of reference does not exist,
     * we throw.
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
        explicit AssetManager(s_ptr<DataStorage> assetDataStorage);

        /**
         * @brief Register a new asset type at the asset manager
         *
         * Sets up all default traits for `T`.
         *
         * @param u_ptr<AssetRegistryModule<T>> assetModule The device module
         *        implementation for `T`. Must not be nullptr.
         *
         * @throw std::invalid_argument if `assetModule` is nullptr.
         * @throw std::out_of_range if `T` has already been registered.
         */
        template<AssetBaseType T>
        void registerAssetType(u_ptr<AssetRegistryModule<T>> assetModule);


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
         * @return TypedAssetID<T> If an asset with the same path already exists
         *         when this method is called, this is the existing asset's ID.
         *         Returns nullopt if the data storage does not contain an
         *         object at `path`.
         * @throw std::invalid_argument if an asset exists at `path` and is
         *                              not of type `T`.
         */
        template<AssetBaseType T>
        auto create(const AssetPath& path) -> std::optional<TypedAssetID<T>>;

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
         *         Returns nullopt if the data storage does not contain an
         *         object at `path`.
         *
         * @throw if the type specified in the stored metadata is not registered
         *        at the asset manager.
         */
        auto create(const AssetPath& path) -> std::optional<AssetID>;

        /**
         * @brief Remove an asset from the asset manager
         *
         * Remove an asset and destroy all associated resources, particularly
         * the asset's device data.
         *
         * This version of `destroy` uses the `ManagerTraits` asset trait.
         */
        void destroy(AssetID id);

        /**
         * @brief Remove an asset from the asset manager
         *
         * Remove an asset and destroy all associated resources, particularly
         * the asset's device data.
         *
         * This version of `destroy` uses the `ManagerTraits` asset trait.
         */
        void destroy(const AssetPath& path);


        ///////////////
        //  Queries  //
        ///////////////

        using AssetManagerBase::getAs;
        using AssetManagerBase::getMetadata;

        bool exists(const AssetPath& path) const;

        template<AssetBaseType T>
        auto getAs(const AssetPath& path) const -> std::optional<TypedAssetID<T>>;

        auto getMetadata(const AssetPath& path) const -> const AssetMetadata&;


        //////////////////////////////////
        //  Access to the data storage  //
        //////////////////////////////////

        /**
         * @return AssetStorage& The storage object that the AssetManager
         *                       uses to store data at asset paths.
         */
        auto getDataStorage() -> AssetStorage&;


        ////////////////////
        //  Asset traits  //
        ////////////////////

        /**
         * @brief Access an implementation of a trait for an asset type
         */
        template<AssetTraitT Trait, AssetBaseType Asset>
        auto getTrait() noexcept
            -> std::optional<std::reference_wrapper<Trait>>
        {
            return getTrait<Trait>(AssetType::make<Asset>());
        }

        /**
         * @brief Access an implementation of a trait for an asset type
         */
        template<AssetTraitT Trait>
        auto getTrait(const AssetType& type) noexcept
            -> std::optional<std::reference_wrapper<Trait>>
        {
            return assetTraits.getTrait<Trait>(type);
        }

        /**
         * @brief Register a trait implementation
         *
         * For an asset type A (e.g. trc::Geometry), register an implementation
         * of an asset trait type T.
         *
         * @tparam T The asset trait type for which to register an
         *         implementation. This parameter must be specified explicitly.
         * @tparam Asset The asset type for which to register the trait
         *         implementation.
         * @tparam Impl Deduced from function parameter `trait`.
         *
         * @param u_ptr<Impl> trait The implementation of trait type `T`. Must
         *        not be nullptr.
         *
         * @throw std::invalid_argument if `trait` is nullptr.
         * @throw std::out_of_range if `Asset` already has an implementation of
         *        `T`.
         *
         * # Example
         * @code
         *  class Greeter : public trc::AssetTrait
         *  {
         *  public:
         *      virtual auto greet() -> std::string = 0;
         *  }
         *
         *  class GeometryGreeter : public Greeter
         *  {
         *  public:
         *      auto greet() -> std::string override {
         *          return "Hello! I am geometry :D";
         *      }
         *  };
         *
         *  const auto geoType = AssetType::make<trc::Geometry>();
         *  manager.registerTrait<Greeter>(geoType, std::make_unique<GeometryGreeter>());
         *
         *  std::cout << manager.getTrait<Greeter>(geoType).greet();
         * @endcode
         */
        template<AssetTraitT T, AssetBaseType Asset, std::derived_from<T> Impl>
        void registerTrait(u_ptr<Impl> trait)
        {
            const AssetType type = AssetType::make<Asset>();
            if (assetTraits.getTrait<T>(type).has_value())
            {
                throw std::out_of_range(
                    "[In AssetManager::registerTrait]: Asset type " + type.getName()
                    + " already has an implementation of trait T [T = " + typeid(T).name() + "]"
                );
            }

            assetTraits.registerTrait<T>(type);
        }

    private:
        template<AssetBaseType T>
        static void registerDefaultTraits(TraitStorage& traits);

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
    inline auto AssetManager::create(const AssetPath& path) -> std::optional<TypedAssetID<T>>
    {
        if (auto it = pathsToAssets.find(path);
            it != pathsToAssets.end())
        {
            try {
                return *getAs<T>(it->second);
            }
            catch (const std::bad_optional_access&)
            {
                throw std::invalid_argument(
                    "[In AssetManager::create(const AssetPath&)]: Tried to create asset of type "
                    + AssetType::make<T>().getName() + " at " + path.string() + ", but asset with"
                    " type " + getAssetType(it->second).getName() + " already exists at this path."
                );
            }
        }

        auto source = dataStorage.loadDeferred<T>(path);
        if (!source)
        {
            log::debug << "[In AssetManager::create(const AssetPath&)]: Unable to load data from"
                          " data storage (path: " << path.string() << ");"
                          " returning std::nullopt.\n";
            return std::nullopt;
        }

        const auto id = create<T>(std::move(*source));
        pathsToAssets.emplace(path, id.getAssetID());

        return id;
    }

    template<AssetBaseType T>
    inline auto AssetManager::getAs(const AssetPath& path) const -> std::optional<TypedAssetID<T>>
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
    void AssetManager::registerAssetType(u_ptr<AssetRegistryModule<T>> assetModule)
    {
        getDeviceRegistry().addModule<T>(std::move(assetModule));
        registerDefaultTraits<T>(assetTraits);
    }

    template<AssetBaseType T>
    inline void AssetManager::registerDefaultTraits(TraitStorage& traits)
    {
        const AssetType type = AssetType::make<T>();
        traits.registerTrait<ManagerTraits>(type, std::make_unique<ManagerTraitsImpl<T>>());
    }
} // namespace trc
