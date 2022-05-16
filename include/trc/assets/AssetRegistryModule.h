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
     * @brief Interface for communication between asset registry and module
     */
    class AssetRegistryModuleInterface
    {
    public:
        AssetRegistryModuleInterface() = default;
        virtual ~AssetRegistryModuleInterface() = default;

        virtual void update(vk::CommandBuffer cmdBuf, FrameRenderState& state) = 0;
    };

    /**
     * @brief Implementation helper
     */
    template<AssetBaseType AssetType>
    class AssetRegistryModuleCacheCrtpBase : public AssetRegistryModuleInterface
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

            void inc();
            void dec();

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

    template<typename T>
    concept AssetRegistryModuleType = requires {
        typename T::Handle;
        requires std::derived_from<T, AssetRegistryModuleInterface>;
    };



    ///////////////////////
    //  Implementations  //
    ///////////////////////

    template<AssetBaseType AssetType>
    void AssetRegistryModuleCacheCrtpBase<AssetType>::CacheRefCounter::inc()
    {
        if (++count == 1) {
            registry->load(asset);
        }
    }

    template<AssetBaseType AssetType>
    void AssetRegistryModuleCacheCrtpBase<AssetType>::CacheRefCounter::dec()
    {
        assert(count > 0);
        if (--count == 0) {
            registry->unload(asset);
        }
    }
} // namespace trc
