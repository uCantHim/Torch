#pragma once

#include "AssetBase.h"

namespace trc
{
    template<typename T>
    class SharedCacheReference
    {
    public:
        SharedCacheReference(T& cacheItem)
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
            if (cache != other.cache)
            {
                cache->dec();
                cache = other.cache;
            }
            cache->inc();
            return *this;
        }
        auto operator=(SharedCacheReference&& other) noexcept -> SharedCacheReference&
        {
            if (cache != other.cache)
            {
                cache->dec();
                cache = other.cache;
            }
            cache->inc();
            return *this;
        }

        ~SharedCacheReference()
        {
            cache->dec();
        }

    protected:
        T* cache;
    };
} // namespace trc
