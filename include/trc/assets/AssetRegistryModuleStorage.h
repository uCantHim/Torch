#pragma once

#include <concepts>
#include <functional>
#include <mutex>

#include <componentlib/Table.h>

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
         */
        template<AssetBaseType T, typename ...Args>
            requires std::constructible_from<AssetRegistryModule<T>, Args...>
                  && std::derived_from<AssetRegistryModule<T>, AssetRegistryModuleInterface<T>>
        void addModule(Args&&... args);

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
        struct StaticIndexPool
        {
            static inline ui32 nextIndex{ 0 };
        };

        template<typename T>
        struct StaticIndex
        {
            static inline const ui32 index{ StaticIndexPool::nextIndex++ };
        };

        std::mutex entriesLock;
        componentlib::Table<u_ptr<AssetRegistryModuleInterfaceCommon>> entries;
    };



    template<AssetBaseType T, typename ...Args>
        requires std::constructible_from<AssetRegistryModule<T>, Args...>
              && std::derived_from<AssetRegistryModule<T>, AssetRegistryModuleInterface<T>>
    void AssetRegistryModuleStorage::addModule(Args&&... args)
    {
        std::scoped_lock lock(entriesLock);
        if (hasModule<T>()) {
            throw std::out_of_range("[In AssetRegistryModuleStorage::addModule]: A module for this"
                                    " type already exitsts.");
        }

        entries.emplace(
            StaticIndex<T>::index,
            std::make_unique<AssetRegistryModule<T>>(std::forward<Args>(args)...)
        );
    }

    template<AssetBaseType T>
    bool AssetRegistryModuleStorage::hasModule() const
    {
        return entries.has(StaticIndex<T>::index)
            && entries.get(StaticIndex<T>::index) != nullptr;
    }

    template<AssetBaseType T>
    auto AssetRegistryModuleStorage::get() -> AssetRegistryModuleInterface<T>&
    {
        std::scoped_lock lock(entriesLock);
        if (!hasModule<T>())
        {
            throw std::out_of_range(
                "[In AssetRegistryModuleStorage::get]: Requested asset registry module type (static"
                " index " + std::to_string(StaticIndex<T>::index) + ") does not exist in the module"
                " storage!"
            );
        }

        return dynamic_cast<AssetRegistryModuleInterface<T>&>(*entries.get(StaticIndex<T>::index));
    }
} // namespace trc
