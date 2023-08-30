#pragma once

#include <concepts>
#include <functional>
#include <mutex>

#include <componentlib/Table.h>
#include <trc_util/data/TypeMap.h>

#include "trc/Types.h"
#include "trc/assets/AssetRegistryModule.h"

namespace trc
{
    /**
     * @brief Stores asset registry modules
     *
     * Hides implementations behind the virtual `AssetRegistryModuleInterface<>`
     * interface, which ensures correct typing but still allows handling
     * incomplete types. This is necessary to allow modules to be defined after
     * asset storage, -registry, and -manager.
     *
     * Also, the virtualness effectively enforces a common interface for asset
     * registry modules.
     */
    class AssetRegistryModuleStorage
    {
    public:
        AssetRegistryModuleStorage() = default;

        /**
         * @brief Register a module for T
         *
         * @param assetModuleImpl The module to register. Must not be nullptr.
         *
         * @throw std::out_of_range if a module for asset type `T` is already
         *        registered.
         * @throw std::invalid_argument if `assetModuleImpl` is nullptr.
         */
        template<AssetBaseType T>
            requires std::derived_from<AssetRegistryModule<T>, AssetRegistryModuleInterface<T>>
        void addModule(u_ptr<AssetRegistryModule<T>> assetModuleImpl);

        /**
         * @return bool True if a module for type T has been registered.
         */
        template<AssetBaseType T>
        bool hasModule() const;

        /**
         * @throw std::out_of_range if no module for T has been registered.
         */
        template<AssetBaseType T>
        auto get() -> AssetRegistryModuleInterface<T>&;

        /**
         * @brief Update all registered modules.
         */
        void update(vk::CommandBuffer cmdBuf, FrameRenderState& state);

    private:
        using TypeIndex = data::TypeIndexAllocator<AssetRegistryModuleStorage>;

        std::mutex entriesLock;
        componentlib::Table<u_ptr<AssetRegistryModuleInterfaceCommon>> entries;
    };



    template<AssetBaseType T>
        requires std::derived_from<AssetRegistryModule<T>, AssetRegistryModuleInterface<T>>
    void AssetRegistryModuleStorage::addModule(u_ptr<AssetRegistryModule<T>> assetModuleImpl)
    {
        if (assetModuleImpl == nullptr) {
            throw std::invalid_argument("[In AssetRegistryModuleStorage::addModule]: The argument"
                                        " `assetModuleImpl` must not be nullptr.");
        }

        std::scoped_lock lock(entriesLock);
        if (hasModule<T>()) {
            throw std::out_of_range("[In AssetRegistryModuleStorage::addModule]: A module for this"
                                    " type already exitsts.");
        }

        entries.emplace(TypeIndex::get<T>(), std::move(assetModuleImpl));
    }

    template<AssetBaseType T>
    bool AssetRegistryModuleStorage::hasModule() const
    {
        return entries.contains(TypeIndex::get<T>())
            && entries.get(TypeIndex::get<T>()) != nullptr;
    }

    template<AssetBaseType T>
    auto AssetRegistryModuleStorage::get() -> AssetRegistryModuleInterface<T>&
    {
        std::scoped_lock lock(entriesLock);
        if (!hasModule<T>())
        {
            throw std::out_of_range(
                "[In AssetRegistryModuleStorage::get]: Requested asset registry module type (static"
                " index " + std::to_string(TypeIndex::get<T>()) + ") does not exist in the module"
                " storage!"
            );
        }

        return dynamic_cast<AssetRegistryModuleInterface<T>&>(*entries.get(TypeIndex::get<T>()));
    }
} // namespace trc
