#pragma once

#include "AssetID.h"
#include "AssetPath.h"
#include "AssetManagerInterface.h"

namespace trc
{
    /**
     * @brief Reference to a specific asset type
     *
     * References are always optional, i.e. they can point to no object.
     *
     * If the reference is set and points to an asset, it only requires to
     * have the referencee's unique path. The corresponding asset ID is
     * filled in when the asset is loaded by the AssetManager. It is
     * possible to query if this has already happened with `hasResolvedID()`.
     */
    template<AssetBaseType T>
    class AssetReference
    {
    public:
        AssetReference(const AssetReference&) = default;
        AssetReference(AssetReference&&) noexcept = default;
        auto operator=(const AssetReference&) -> AssetReference& = default;
        auto operator=(AssetReference&&) noexcept -> AssetReference& = default;
        ~AssetReference() = default;

        AssetReference() = default;
        AssetReference(AssetPath path);
        AssetReference(TypedAssetID<T> id);

        /**
         * A reference is empty if it points to neither an asset path nor
         * an asset ID.
         *
         * @return bool
         */
        bool empty() const;

        /**
         * Tests whether the reference points to an object that is
         * identified by an asset path, or whether it is an ad-hoc
         * reference to an asset that exists only in memory.
         *
         * @return bool True if the referenced object is identified by an
         *              asset path.
         */
        bool hasAssetPath() const;
        auto getAssetPath() const -> const AssetPath&;

        /**
         * @return bool True if the reference points to an existing asset
         *              with an ID.
         */
        bool hasResolvedID() const;
        auto getID() const -> TypedAssetID<T>;

        /**
         * Queries the ID of the asset at the referenced path from the
         * asset manager and stores it the reference. Loads the referenced
         * asset if it has not been loaded before.
         */
        void resolve(AssetManagerFwd& manager);

    private:
        struct SharedData
        {
            std::optional<AssetPath> path;
            TypedAssetID<T> id;
        };

        s_ptr<SharedData> data;
    };



    ///////////////////////////
    //    Implementations    //
    ///////////////////////////

    template<AssetBaseType T>
    AssetReference<T>::AssetReference(AssetPath path)
        : data(new SharedData{ .path=std::move(path), .id={} })
    {
    }

    template<AssetBaseType T>
    AssetReference<T>::AssetReference(TypedAssetID<T> id)
        : data(new SharedData{ .path=std::nullopt, .id=id })
    {
    }

    template<AssetBaseType T>
    bool AssetReference<T>::empty() const
    {
        return data == nullptr;
    }

    template<AssetBaseType T>
    bool AssetReference<T>::hasAssetPath() const
    {
        return data != nullptr && data->path.has_value();
    }

    template<AssetBaseType T>
    auto AssetReference<T>::getAssetPath() const -> const AssetPath&
    {
        if (!hasAssetPath())
        {
            throw std::runtime_error("[In AssetReference::getAssetPath]: Reference does not"
                                     " contain an asset path!");
        }

        return data->path.value();
    }

    template<AssetBaseType T>
    void AssetReference<T>::resolve(AssetManagerFwd& manager)
    {
        if (!hasResolvedID())
        {
            if (empty())
            {
                throw std::runtime_error("[In AssetReference::resolve]: Tried to resolve an empty"
                                         " asset reference");
            }

            const auto& path = data->path.value();
            if (!manager.exists(path)) {
                manager.load<T>(path);
            }
            data->id = manager.template getAsset<T>(path);
        }
    }

    template<AssetBaseType T>
    bool AssetReference<T>::hasResolvedID() const
    {
        return data != nullptr && data->id.uniqueId != AssetID::NONE;
    }

    template<AssetBaseType T>
    auto AssetReference<T>::getID() const -> TypedAssetID<T>
    {
        if (!hasResolvedID())
        {
            throw std::runtime_error("[In AssetReference::getID]: The reference does not point to"
                                     " a valid asset ID");
        }

        return data->id;
    }
} // namespace trc
