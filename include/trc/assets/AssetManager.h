#pragma once

#include <concepts>
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

namespace trc
{
    /**
     * It is important to note that, while `AssetManager` accesses a
     * `DataStorage` to load asset data, it does not manage objects in the
     * storage. It never writes new objects to the storage, nor does it delete
     * objects from it. Thus, the `AssetPath`-based interface always expects
     * objects at the specified path to be available in the storage (i.e. the
     * user must have added them via `AssetStorage::store` beforehand).
     * Likewise, `AssetManager::destroy(AssetPath)` merely removes the reference
     * to the asset (`AssetManager` 'forgets' about the asset), but does not
     * delete data in the storage.
     */
    class AssetManager
    {
    public:
        using const_iterator = AssetManagerBase::const_iterator;

        using AssetInfo = AssetManagerBase::AssetInfo;

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

        /**
         * @brief Create an asset
         *
         * Registers an asset at the asset manager.
         *
         * An asset is defined by an asset source object, which is a mechanism
         * to load an asset's data and metadata lazily.
         *
         * Whether or not the data is actually loaded lazily or immediately when
         * `AssetManagerBase::create` is called is determined by the underlying
         * `AssetRegistryModule` implementation for the asset type `T`.
         *
         * @tparam T The type of asset to create.
         * @param dataSource A source for the new asset's data. Must
         *        not be nullptr.
         *
         * @throw std::invalid_argument if `dataSource == nullptr`.
         */
        template<AssetBaseType T>
        auto create(u_ptr<AssetSource<T>> dataSource) -> TypedAssetID<T>;

        /**
         * @brief Create an asset
         *
         * Registers an asset at the asset manager.
         *
         * An asset is defined by an asset source object, which is a mechanism
         * to load an asset's data and metadata lazily.
         *
         * Whether or not the data is actually loaded lazily or immediately when
         * `AssetManagerBase::create` is called is determined by the underlying
         * `AssetRegistryModule` implementation for the asset type `T`.
         *
         * @tparam T The type of asset to create.
         * @param dataSource A source for the new asset's data. Must
         *        not be nullptr.
         *
         * @throw std::invalid_argument if `dataSource == nullptr`.
         */
        template<AssetBaseType T, template<typename> typename S>
            requires std::derived_from<S<T>, AssetSource<T>>
        auto create(u_ptr<S<T>> dataSource) -> TypedAssetID<T> {
            return create(u_ptr<AssetSource<T>>{ std::move(dataSource) });
        }

        /**
         * @brief Create an asset from in-memory data
         *
         * Asset data imported by this method will permanently stay in host
         * memory until the asset is destroyed.
         *
         * @throw std::out_of_range if no device registry module for asset type
         *                          `T` is registered.
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
         * @throw std::invalid_argument if an asset at `path` is already registered
         *                              and is not of type `T`.
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
         * @throw std::out_of_range if the type specified in the stored metadata
         *                          is not registered at the asset manager.
         */
        auto create(const AssetPath& path) -> std::optional<AssetID>;

        /**
         * @brief Remove an asset from the asset manager
         *
         * Remove an asset and destroy all associated resources, particularly
         * the asset's device data.
         */
        void destroy(AssetID id);

        /**
         * @brief Remove an asset from the asset manager
         *
         * Remove an asset and destroy all associated resources, particularly
         * the asset's device data.
         *
         * @throw InvalidAssetIdError if `id` is invalid.
         */
        template<AssetBaseType T>
        void destroy(TypedAssetID<T> id);

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

        bool exists(const AssetPath& path) const;

        /**
         * @brief Retrieve an asset's dynamic type information
         *
         * A shortcut for `assetManager.getMetadata(id).type`.
         *
         * @throw InvalidAssetIdError if `id` is invalid.
         */
        auto getAssetType(AssetID id) const -> const AssetType&;

        /**
         * @brief Try to cast a typeless asset ID to a typed ID
         *
         * @return optional<TypedAssetID<T>> A TypedAssetID if the asset at `id`
         *         is indeed of the specified type `T`, or nullopt otherwise.
         * @throw InvalidAssetIdError if `id` is invalid.
         */
        template<AssetBaseType T>
        auto getAs(AssetID id) const -> std::optional<TypedAssetID<T>> {
            return base.getAs<T>(id);
        }

        /**
         * @brief Get an asset from its asset path
         *
         * @tparam T Try to interpret the asset as `T`. Returns std::nullopt if
         *           the data at `path` is of another type.
         *
         * @return std::optional<TypedAssetID> Returns an ID if an asset with
         *         asset path `path` exists and its metadata indicated that it
         *         is of the specified type `T`. Returns std::nullopt otherwise.
         */
        template<AssetBaseType T>
        auto getAs(const AssetPath& path) const -> std::optional<TypedAssetID<T>>;

        /**
         * @throw InvalidAssetIdError if `id` is invalid.
         */
        auto getMetadata(AssetID id) const -> const AssetMetadata& {
            return base.getMetadata(id);
        }

        /**
         * @return const AssetMetadata* nullptr if nothing exists at `path`.
         */
        auto getMetadata(const AssetPath& path) const -> const AssetMetadata*;

        /**
         * @brief Get a handle to an asset's device resources
         *
         * Forces the underlying `AssetRegistryModule` of asset type `T` to load
         * the asset's data and create a device representation if the asset is
         * loaded lazily.
         *
         * @return AssetHandle<T> the respective device implementation of asset
         *         type `T`.
         * @throw InvalidAssetIdError if `id` is invalid.
         */
        template<AssetBaseType T>
        auto getHandle(TypedAssetID<T> id) -> AssetHandle<T> {
            return base.getHandle(id);
        }

        auto begin() const -> const_iterator;
        auto end() const -> const_iterator;


        //////////////////////////////////
        //  Access to the data storage  //
        //////////////////////////////////

        /**
         * @return AssetStorage& The storage object that the AssetManager
         *                       uses to store data at asset paths.
         */
        auto getDataStorage() -> AssetStorage&;


        ////////////////////////////////////////
        //  Access to device implementations  //
        ////////////////////////////////////////

        /**
         * @throw std::out_of_range if no module for asset type `T` is
         *        registered.
         */
        template<AssetBaseType T>
        auto getModule() -> AssetRegistryModule<T>& {
            return base.getModule<T>();
        }

        /**
         * @brief Access the device-data registry
         *
         * One should normally not access the device registry directly. Use the
         * asset manager's interface to manipulate assets instead.
         *
         * Note: It's a very bad idea to register modules directly at the device
         * registry - the asset manager has to know about them! Use
         * AssetManager::registerAssetType instead.
         */
        auto getDeviceRegistry() -> AssetRegistry&;


        ////////////////////
        //  Asset traits  //
        ////////////////////

        /**
         * @brief Access an implementation of a trait for an asset type
         *
         * @return Trait* nullptr if no implementation of `Trait` is registered
         *                for type `Asset`.
         */
        template<AssetTraitT Trait, AssetBaseType Asset>
        auto getTrait() noexcept -> Trait*
        {
            return getTrait<Trait>(AssetType::make<Asset>());
        }

        /**
         * @brief Access an implementation of a trait for an asset type
         *
         * @return Trait* nullptr if no implementation of `Trait` is registered
         *                for `type`.
         */
        template<AssetTraitT Trait>
        auto getTrait(const AssetType& type) noexcept -> Trait*
        {
            if (auto trait = assetTraits.getTrait<Trait>(type)) {
                return &trait.value().get();
            }
            return nullptr;
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

            assetTraits.registerTrait<T>(type, std::move(trait));
        }

    private:
        /**
         * @brief An internal asset source used by the AssetManagerBase
         *
         * First resolves any asset references before loading an asset's data.
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
        static void registerDefaultTraits(TraitStorage& traits);

        template<AssetBaseType T>
        void resolveReferences(AssetData<T>& data);

        AssetManagerBase base;

        AssetStorage dataStorage;
        TraitStorage assetTraits;

        std::unordered_map<AssetPath, AssetID> pathsToAssets;
        util::SafeVector<AssetPath> assetsToPaths;
    };



    template<AssetBaseType T>
    auto AssetManager::create(u_ptr<AssetSource<T>> dataSource) -> TypedAssetID<T>
    {
        if (dataSource == nullptr) {
            throw std::invalid_argument("[In AssetManager::create(AssetSource)]: Argument "
                                        " `dataSource` must not be nullptr!");
        }

        // Create the internal reference-resolving source
        u_ptr<AssetSource<T>> wrapper = std::make_unique<InternalAssetSource<T>>(
            *this,
            std::move(dataSource)
        );

        return base.create(std::move(wrapper));
    }

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
                return getAs<T>(it->second).value();
            }
            catch (const std::bad_optional_access&)
            {
                log::error << "[In AssetManager::create(const AssetPath&)]: Tried to create asset"
                           << " of type " << AssetType::make<T>().getName()
                           << " at " << path.string()
                           << ", but an asset of different type " << getAssetType(it->second).getName()
                           << " already exists at this path.";
                return std::nullopt;
            }
        }

        auto source = dataStorage.loadDeferred<T>(path);
        if (!source)
        {
            log::error << "[In AssetManager::create(const AssetPath&)]: Unable to load data from"
                       << " data storage: " << source.error().message;
            return std::nullopt;
        }

        try {
            const auto id = create<T>(std::move(*source));
            pathsToAssets.emplace(path, id.getAssetID());
            assetsToPaths.emplace(base.toIndex(id.getAssetID()), path);

            return id;
        }
        catch (const std::exception& err) {
            log::error << "[In AssetManager::create(const AssetPath&)] Unable to create asset: "
                       << err.what();
        }
        return std::nullopt;
    }

    template<AssetBaseType T>
    inline void AssetManager::destroy(TypedAssetID<T> id)
    {
        try {
            const ui32 idx = base.toIndex(id);
            const auto path = assetsToPaths.copyAtomically(idx);
            assetsToPaths.erase(idx);
            pathsToAssets.erase(path);
        }
        catch (const std::out_of_range&) {
            // Asset with the specified ID does not exist (copyAtomically failed).
            // Perhaps I should throw an InvalidAssetIdError here, but I don't know
            // the implications of this so I won't.
        }

        base.destroy(id);
    }

    template<AssetBaseType T>
    inline auto AssetManager::getAs(const AssetPath& path) const -> std::optional<TypedAssetID<T>>
    {
        auto it = pathsToAssets.find(path);
        if (it == pathsToAssets.end()) {
            return std::nullopt;
        }

        try {
            return getAs<T>(it->second).value();
        }
        catch (const std::bad_optional_access&) {
            return std::nullopt;
        }
    }

    template<AssetBaseType T>
    void AssetManager::resolveReferences(AssetData<T>& data)
    {
        if constexpr (requires{ data.resolveReferences(*this); }) {
            data.resolveReferences(*this);
        }
    }
} // namespace trc

#include "trc/assets/DefaultTraits.h"

namespace trc
{
    template<AssetBaseType T>
    void AssetManager::registerAssetType(u_ptr<AssetRegistryModule<T>> assetModule)
    {
        base.getDeviceRegistry().addModule<T>(std::move(assetModule));

        // Initialize the module
        auto& mod = base.getDeviceRegistry().getModule<T>();
        mod.parent = this;
        mod.init(*this);

        // Create asset manager create/destroy traits
        registerDefaultTraits<T>(assetTraits);
    }

    template<AssetBaseType T>
    inline void AssetManager::registerDefaultTraits(TraitStorage& traits)
    {
        const AssetType type = AssetType::make<T>();
        try {
            traits.registerTrait<ManagerTraits>(type, std::make_unique<ManagerTraitsImpl<T>>());
        }
        catch (const std::out_of_range&) {
            log::warn << "[In AssetManager::registerDefaultTraits]: An implementation of"
                         " ManagerTraits is already registered for asset type " + type.getName()
                         + ". The default implementation will not be registered.";
        }
    }
} // namespace trc
