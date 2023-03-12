#pragma once

#include <cassert>

#include <any>
#include <optional>
#include <stdexcept>

#include <trc_util/data/IdPool.h>
#include <trc_util/data/SafeVector.h>

#include "trc/assets/AssetBase.h"
#include "trc/assets/AssetRegistry.h"
#include "trc/assets/AssetSource.h"
#include "trc/assets/AssetType.h"

namespace trc
{
    class AssetManagerBase;

    struct _AssetIdTypeTag {};

    /**
     * @brief ID for basic metadata that is shared by all asset types
     */
    using AssetID = data::HardTypesafeID<_AssetIdTypeTag, AssetManagerBase, ui32>;

    /**
     * @brief ID for a specific asset type
     */
    template<AssetBaseType T>
    struct TypedAssetID
    {
    public:
        using LocalID = typename AssetBaseTypeTraits<T>::LocalID;
        using Handle = typename AssetBaseTypeTraits<T>::Handle;

        /** Construct a valid ID */
        TypedAssetID(AssetID base, LocalID type, AssetManagerBase& man)
            : uniqueId(base), id(type), manager(&man)
        {}

        operator AssetID() const;

        bool operator==(const TypedAssetID&) const noexcept;
        bool operator!=(const TypedAssetID&) const noexcept = default;

        ////////////////
        //  Asset access

        /** @return AssetID Global ID for type-agnostic asset data */
        auto getAssetID() const -> AssetID;
        /** @return AssetID Local ID for type-specific asset data */
        auto getDeviceID() const -> LocalID;

        auto get() const -> Handle;
        auto getDeviceDataHandle() const -> Handle;
        auto getMetadata() const -> const AssetMetadata&;

        auto getAssetManager() -> AssetManagerBase&;
        auto getModule() -> AssetRegistryModule<T>&;

    private:
        /** ID that references metadata common to all assets */
        AssetID uniqueId;

        /** ID that references data in the asset type's specific asset registry */
        LocalID id;

        AssetManagerBase* manager;
    };

    /**
     * @brief Manages existence of assets at a very basic level
     *
     * Does not deal with asset data or device representations. Only tracks the
     * plain existence of assets and their associated data sources.
     *
     * Enforces strongly typed interfaces where required. Abstractions can be
     * built that handle typing automatically (e.g. via `AssetType`).
     */
    class AssetManagerBase
    {
    public:
        AssetManagerBase(const AssetManagerBase&) = delete;
        AssetManagerBase(AssetManagerBase&&) noexcept = delete;
        AssetManagerBase& operator=(const AssetManagerBase&) = delete;
        AssetManagerBase& operator=(AssetManagerBase&&) noexcept = delete;

        ~AssetManagerBase() = default;

        AssetManagerBase(const Instance& instance,
                         const AssetRegistryCreateInfo& deviceRegistryCreateInfo);

        /**
         * @throw std::invalid_argument if `dataSource == nullptr`.
         */
        template<AssetBaseType T>
        auto create(u_ptr<AssetSource<T>> dataSource) -> TypedAssetID<T>;

        /**
         * @throw std::invalid_argument if the existing asset with ID `id` is
         *                              not of type `T`.
         */
        template<AssetBaseType T>
        void destroy(AssetID id);

        template<AssetBaseType T>
        auto getAs(AssetID id) const -> std::optional<TypedAssetID<T>>;
        template<AssetBaseType T>
        auto getHandle(TypedAssetID<T> id) -> AssetHandle<T>;

        auto getMetadata(AssetID id) const -> const AssetMetadata&;

        /**
         * @brief Retrieve an asset's dynamic type information
         *
         * A shortcut for `assetManager.getMetadata(id).type`.
         */
        auto getAssetType(AssetID id) const -> const AssetType&;

        template<AssetBaseType T>
        auto getModule() -> AssetRegistryModule<T>&;

        auto getDeviceRegistry() -> AssetRegistry&;

    private:
        struct AssetInfo
        {
            template<AssetBaseType T>
            AssetInfo(AssetMetadata meta, TypedAssetID<T> id)
                : metadata(std::move(meta)), typedId(id)
            {}

            template<AssetBaseType T>
            auto asType() const -> std::optional<TypedAssetID<T>>
            {
                try {
                    return std::any_cast<TypedAssetID<T>>(typedId);
                }
                catch (const std::bad_any_cast&) {
                    return std::nullopt;
                }
            }

            auto getMetadata() const -> const AssetMetadata& {
                return metadata;
            }

        private:
            AssetMetadata metadata;
            std::any typedId;
        };

        /**
         * This is the asset source passed to the asset registry.
         *
         * It resolves any asset references when loading its data.
         */
        template<AssetBaseType T>
        class InternalAssetSource : public AssetSource<T>
        {
        public:
            InternalAssetSource(AssetManagerBase& man, u_ptr<AssetSource<T>> impl)
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
            AssetManagerBase* manager;
            u_ptr<AssetSource<T>> source;
        };

        template<AssetBaseType T>
        void resolveReferences(AssetData<T>& data);

        data::IdPool<ui32> assetIdPool;
        util::SafeVector<AssetInfo> assetInformation;

        AssetRegistry deviceRegistry;
    };



    template<AssetBaseType T>
    TypedAssetID<T>::operator AssetID() const
    {
        return uniqueId;
    }

    template<AssetBaseType T>
    bool TypedAssetID<T>::operator==(const TypedAssetID& other) const noexcept
    {
        assert(manager != nullptr);
        return uniqueId == other.uniqueId && manager == other.manager;
    }

    template<AssetBaseType T>
    auto TypedAssetID<T>::getAssetID() const -> AssetID
    {
        return uniqueId;
    }

    template<AssetBaseType T>
    auto TypedAssetID<T>::getDeviceID() const -> LocalID
    {
        return id;
    }

    template<AssetBaseType T>
    auto TypedAssetID<T>::get() const -> Handle
    {
        return getDeviceDataHandle();
    }

    template<AssetBaseType T>
    auto TypedAssetID<T>::getDeviceDataHandle() const -> Handle
    {
        assert(manager != nullptr);
        return manager->getHandle(*this);
    }

    template<AssetBaseType T>
    auto TypedAssetID<T>::getMetadata() const -> const AssetMetadata&
    {
        return manager->getMetadata(uniqueId);
    }

    template<AssetBaseType T>
    auto TypedAssetID<T>::getAssetManager() -> AssetManagerBase&
    {
        assert(manager != nullptr);
        return *manager;
    }

    template<AssetBaseType T>
    auto TypedAssetID<T>::getModule() -> AssetRegistryModule<T>&
    {
        assert(manager != nullptr);
        return manager->getModule<T>();
    }



    template<AssetBaseType T>
    auto AssetManagerBase::create(u_ptr<AssetSource<T>> _dataSource) -> TypedAssetID<T>
    {
        if (_dataSource == nullptr) {
            throw std::invalid_argument("[In AssetManagerBase::create]: Argument `dataSource` must"
                                        " not be nullptr!");
        }
        using LocalID = typename AssetBaseTypeTraits<T>::LocalID;

        u_ptr<AssetSource<T>> source = std::make_unique<InternalAssetSource<T>>(
            *this,
            std::move(_dataSource)
        );
        AssetMetadata meta = source->getMetadata();

        const AssetID id{ assetIdPool.generate() };
        const LocalID localId = deviceRegistry.add(std::move(source));
        const TypedAssetID<T> typedId{ id, localId, *this };

        assetInformation.emplace(
            ui32{id},
            AssetInfo{ std::move(meta), typedId }
        );

        return typedId;
    }

    template<AssetBaseType T>
    void AssetManagerBase::destroy(AssetID id)
    {
        assert(assetInformation.contains(ui32{id}));
        if (!getAs<T>(id))
        {
            throw std::invalid_argument(
                "[In AssetManagerBase]: Tried to remove asset " + std::to_string(ui32{id})
                + " with type " + AssetType::make<T>().getName() + ", but the existing asset has"
                " type " + getAssetType(id).getName() + "."
            );
        }

        const auto typedId = *getAs<T>(id);
        deviceRegistry.remove<T>(typedId.getDeviceID());
        assetInformation.erase(ui32{id});
        assetIdPool.free(ui32{id});
    }

    template<AssetBaseType T>
    auto AssetManagerBase::getAs(AssetID id) const -> std::optional<TypedAssetID<T>>
    {
        assert(assetInformation.contains(ui32{id}));
        return assetInformation.at(ui32{id}).asType<T>();
    }

    template<AssetBaseType T>
    auto AssetManagerBase::getHandle(TypedAssetID<T> id) -> AssetHandle<T>
    {
        assert(assetInformation.contains(ui32{id.getAssetID()}));
        return deviceRegistry.get<T>(id.getDeviceID());
    }

    template<AssetBaseType T>
    auto AssetManagerBase::getModule() -> AssetRegistryModule<T>&
    {
        return dynamic_cast<AssetRegistryModule<T>&>(getDeviceRegistry().getModule<T>());
    }

    template<AssetBaseType T>
    void AssetManagerBase::resolveReferences(AssetData<T>& data)
    {
        if constexpr (requires{ data.resolveReferences(*this); }) {
            data.resolveReferences(*this);
        }
    }
} // namespace trc
