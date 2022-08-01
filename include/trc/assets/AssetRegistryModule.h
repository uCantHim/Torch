#pragma once

#include "VulkanInclude.h"
#include "DescriptorSetUtils.h"
#include "AssetID.h"
#include "SharedDescriptorSet.h"

namespace trc
{
    class FrameRenderState;

    struct AssetRegistryModuleCreateInfo
    {
        const vkb::Device& device;

        SharedDescriptorSet::Builder* layoutBuilder;

        vk::BufferUsageFlags geometryBufferUsage{};
        bool enableRayTracing{ true };
    };

    /**
     * @brief Type-agnostic part of an asset module's interface
     */
    class AssetRegistryModuleInterfaceCommon
    {
    public:
        AssetRegistryModuleInterfaceCommon() = default;
        virtual ~AssetRegistryModuleInterfaceCommon() = default;

        virtual void update(vk::CommandBuffer cmdBuf, FrameRenderState& state) = 0;
    };

    /**
     * @brief Type-specific part of an asset module's interface
     */
    template<AssetBaseType T>
    class AssetRegistryModuleInterface : public AssetRegistryModuleInterfaceCommon
    {
    public:
        using LocalID = typename TypedAssetID<T>::LocalID;
        using Handle = AssetHandle<T>;

        virtual auto add(u_ptr<AssetSource<T>> source) -> LocalID = 0;
        virtual void remove(LocalID id) = 0;

        virtual auto getHandle(LocalID id) -> AssetHandle<T> = 0;
    };

    /**
     * @brief Helper for dynamic asset load/unload mechanisms
     *
     * Defines the protected virtual methods `load` and `unload`.
     */
    template<AssetBaseType AssetType>
    class AssetRegistryModuleCacheCrtpBase : public AssetRegistryModuleInterface<AssetType>
    {
        using Derived = AssetRegistryModule<AssetType>;

    public:
        using LocalID = typename TypedAssetID<AssetType>::LocalID;

    protected:
        /**
         * Can't use concepts for CRTP, so use the constructor to ensure
         * the correct inheritance pattern.
         */
        AssetRegistryModuleCacheCrtpBase()
        {
            static_assert(std::derived_from<Derived, std::remove_reference_t<decltype(*this)>>);
        }

        virtual void load(LocalID id) = 0;
        virtual void unload(LocalID id) = 0;

    protected:
        /**
         * @brief Storage for data, associated with a reference counter
         *
         * Stores a data type T together with a `LocalID` and a reference
         * count. Create references to the data through `SharedCacheItem`.
         */
        template<std::move_constructible T>
        class CacheItem
        {
        public:
            CacheItem() = delete;
            CacheItem(const CacheItem&) = delete;
            CacheItem(CacheItem&&) noexcept = delete;
            CacheItem& operator=(const CacheItem&) = delete;
            CacheItem& operator=(CacheItem&&) noexcept = delete;

            CacheItem(T item, LocalID asset, Derived* reg)
                : item(std::move(item)), asset(asset), registry(reg)
            {}

            ~CacheItem() = default;

            auto getItem() -> T& {
                return item;
            }

            auto getItem() const -> const T& {
                return item;
            }

            void incRefCount()
            {
                if (++count == 1) {
                    registry->load(asset);
                }
            }

            void decRefCount()
            {
                assert(count > 0);
                if (--count == 0) {
                    registry->unload(asset);
                }
            }

        private:
            T item;

            ui32 count{ 0 };
            LocalID asset;
            Derived* registry;
        };

        /**
         * Cannot reference no object, so move operations have copy
         * semantics.
         */
        template<typename T>
        class SharedCacheItem
        {
        public:
            SharedCacheItem(CacheItem<T>& _cacheItem)
                : cacheItem(&_cacheItem)
            {
                cacheItem->incRefCount();
            }

            SharedCacheItem(const SharedCacheItem& other)
                : cacheItem(other.cacheItem)
            {
                cacheItem->incRefCount();
            }

            SharedCacheItem(SharedCacheItem&& other) noexcept
                : cacheItem(other.cacheItem)
            {
                cacheItem->incRefCount();
            }

            ~SharedCacheItem()
            {
                cacheItem->decRefCount();
            }

            auto operator*() -> T& {
                return cacheItem->getItem();
            }

            auto operator*() const -> const T& {
                return cacheItem->getItem();
            }

            auto operator->() -> T* {
                return &cacheItem->getItem();
            }

            auto operator->() const -> const T* {
                return &cacheItem->getItem();
            }

            auto operator=(const SharedCacheItem& other) -> SharedCacheItem&
            {
                if (this != &other)
                {
                    if (cacheItem != other.cacheItem)
                    {
                        cacheItem->decRefCount();
                        cacheItem = other.cacheItem;
                    }
                    cacheItem->incRefCount();
                }
                return *this;
            }

            auto operator=(SharedCacheItem&& other) noexcept -> SharedCacheItem&
            {
                if (this != &other)
                {
                    if (cacheItem != other.cacheItem)
                    {
                        cacheItem->decRefCount();
                        cacheItem = other.cacheItem;
                    }
                    cacheItem->incRefCount();
                }
                return *this;
            }

        protected:
            CacheItem<T>* cacheItem;
        };
    };
} // namespace trc
