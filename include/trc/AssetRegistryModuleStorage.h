#pragma once

#include <cstdint>
#include <memory>
#include <concepts>
#include <functional>

#include <vkb/Device.h>
#include <trc_util/data/IndexMap.h>

#include "AssetRegistryModule.h"

namespace trc
{
    struct AssetRegistryModuleCreateInfo
    {
        const vkb::Device& device;

        vk::BufferUsageFlags geometryBufferUsage{};

        bool enableRayTracing{ true };
    };

    class AssetRegistryModuleStorage
    {
    public:
        explicit AssetRegistryModuleStorage(const AssetRegistryModuleCreateInfo& info);
        ~AssetRegistryModuleStorage() = default;

        template<typename T>
        auto get() -> T&;

        template<typename T>
        auto get() const -> const T&;

        void foreach(std::function<void(AssetRegistryModuleInterface&)> func);

    private:
        struct StaticIndexPool
        {
            static inline uint32_t nextIndex{ 0 };
        };

        template<typename T>
        struct StaticIndex
        {
            static inline const uint32_t index{ StaticIndexPool::nextIndex++ };
            static const bool _init;  // Initialization
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

        struct TypeRegistrations
        {
            template<typename T>
                requires std::is_constructible_v<T, const AssetRegistryModuleCreateInfo&>
            void add();

            data::IndexMap<uint32_t, TypeEntry(*)(const AssetRegistryModuleCreateInfo&)> factories;
        };

        static inline TypeRegistrations types;

        data::IndexMap<uint32_t, TypeEntry> entries;
    };
} // namespace trc

#include "AssetRegistryModuleStorage.inl"
