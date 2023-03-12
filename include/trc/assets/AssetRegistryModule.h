#pragma once

#include "trc/VulkanInclude.h"
#include "trc/assets/AssetBase.h"
#include "trc/assets/AssetSource.h"

namespace trc
{
    class FrameRenderState;

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
        using LocalID = typename AssetBaseTypeTraits<T>::LocalID;
        using Handle = typename AssetBaseTypeTraits<T>::Handle;

        virtual auto add(u_ptr<AssetSource<T>> source) -> LocalID = 0;
        virtual void remove(LocalID id) = 0;

        virtual auto getHandle(LocalID id) -> Handle = 0;
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
        using LocalID = typename AssetBaseTypeTraits<AssetType>::LocalID;

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
         * @brief A reference counter
         *
         * Create shared references through `SharedCacheReference`.
         */
        class ReferenceCounter
        {
        public:
            ReferenceCounter() = delete;
            ReferenceCounter(const ReferenceCounter&) = delete;
            ReferenceCounter(ReferenceCounter&&) noexcept = delete;
            ReferenceCounter& operator=(const ReferenceCounter&) = delete;
            ReferenceCounter& operator=(ReferenceCounter&&) noexcept = delete;

            ReferenceCounter(LocalID asset, Derived* reg)
                : asset(asset), registry(reg)
            {}

            ~ReferenceCounter() = default;

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
            ui32 count{ 0 };
            LocalID asset;
            Derived* registry;
        };

        /**
         * Cannot reference no object, so move operations have copy
         * semantics.
         */
        class SharedCacheReference
        {
        public:
            SharedCacheReference(ReferenceCounter& _cacheItem)
                : cacheItem(&_cacheItem)
            {
                cacheItem->incRefCount();
            }

            SharedCacheReference(const SharedCacheReference& other)
                : cacheItem(other.cacheItem)
            {
                cacheItem->incRefCount();
            }

            SharedCacheReference(SharedCacheReference&& other) noexcept
                : cacheItem(other.cacheItem)
            {
                cacheItem->incRefCount();
            }

            ~SharedCacheReference()
            {
                cacheItem->decRefCount();
            }

            auto operator=(const SharedCacheReference& other) -> SharedCacheReference&
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

            auto operator=(SharedCacheReference&& other) noexcept -> SharedCacheReference&
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
            ReferenceCounter* cacheItem;
        };
    };
} // namespace trc
