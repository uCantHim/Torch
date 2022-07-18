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
     * @brief Implementation helper
     */
    template<AssetBaseType AssetType>
    class AssetRegistryModuleCacheCrtpBase : public AssetRegistryModuleInterface<AssetType>
    {
        using Derived = AssetRegistryModule<AssetType>;

    public:
        using LocalID = typename TypedAssetID<AssetType>::LocalID;

    protected:
        AssetRegistryModuleCacheCrtpBase()
        {
            static_assert(std::derived_from<Derived, std::remove_reference_t<decltype(*this)>>);
        }

        virtual void load(LocalID id) = 0;
        virtual void unload(LocalID id) = 0;

    protected:
        struct CacheRefCounter
        {
            CacheRefCounter(LocalID asset, Derived* reg)
                : asset(asset), registry(reg)
            {}

            void inc()
            {
                if (++count == 1) {
                    registry->load(asset);
                }
            }

            void dec()
            {
                assert(count > 0);
                if (--count == 0) {
                    registry->unload(asset);
                }
            }

            ui32 count{ 0 };
            LocalID asset;
            Derived* registry;
        };

        template<typename CacheItem>
        class SharedCacheReference
        {
        public:
            SharedCacheReference(CacheItem& cacheItem)
                : cache(&cacheItem)
            {
                cache->inc();
            }

            SharedCacheReference(const SharedCacheReference& other)
                : cache(other.cache)
            {
                cache->inc();
            }

            SharedCacheReference(SharedCacheReference&& other) noexcept
                : cache(other.cache)
            {
                cache->inc();
            }

            auto operator=(const SharedCacheReference& other) -> SharedCacheReference&
            {
                if (this != &other)
                {
                    if (cache != other.cache)
                    {
                        cache->dec();
                        cache = other.cache;
                    }
                    cache->inc();
                }
                return *this;
            }

            auto operator=(SharedCacheReference&& other) noexcept -> SharedCacheReference&
            {
                if (this != &other)
                {
                    if (cache != other.cache)
                    {
                        cache->dec();
                        cache = other.cache;
                    }
                    cache->inc();
                }
                return *this;
            }

            ~SharedCacheReference()
            {
                cache->dec();
            }

        protected:
            CacheItem* cache;
        };
    };
} // namespace trc
