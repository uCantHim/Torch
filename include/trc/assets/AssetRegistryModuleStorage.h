#pragma once

#include <concepts>
#include <functional>
#include <mutex>

#include <componentlib/Table.h>

#include "Types.h"
#include "AssetRegistryModule.h"

namespace trc
{
    class AssetRegistryModuleStorage
    {
    public:
        AssetRegistryModuleStorage() = default;
        ~AssetRegistryModuleStorage() = default;

        template<AssetRegistryModuleType T, typename ...Args>
            requires std::constructible_from<T, Args...>
                  && std::derived_from<T, AssetRegistryModuleInterface>
        void addModule(Args&&... args);

        template<AssetRegistryModuleType T>
        auto get() -> T&;

        void foreach(std::function<void(AssetRegistryModuleInterface&)> func);

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

        struct TypeEntry
        {
            TypeEntry(const TypeEntry&) = delete;
            auto operator=(const TypeEntry&) -> TypeEntry& = delete;

            TypeEntry() = default;
            TypeEntry(TypeEntry&& other) noexcept;
            auto operator=(TypeEntry&&) noexcept -> TypeEntry&;

            template<typename T>
            explicit TypeEntry(std::unique_ptr<T> obj);
            ~TypeEntry();

            template<typename T> auto as() -> T&;
            template<typename T> auto as() const -> const T&;

            bool valid() const;

        private:
            void* ptr{ nullptr };
            void(*_delete)(void*){ nullptr };
        };

        std::mutex entriesLock;
        componentlib::Table<TypeEntry> entries;
    };
} // namespace trc

#include "AssetRegistryModuleStorage.inl"
