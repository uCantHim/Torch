#pragma once

#include <filesystem>

#include "AssetPath.h"
#include "assets/AssetDataProxy.h"
#include "assets/InternalFormat.h"

namespace trc
{
    template<AssetBaseType T>
    class AssetSource
    {
    public:
        virtual ~AssetSource() = default;
        virtual auto load() -> AssetData<T> = 0;
    };

    template<AssetBaseType T>
    class AssetPathSource : public AssetSource<T>
    {
    public:
        explicit AssetPathSource(AssetPath path)
            : path(std::move(path))
        {}

        auto load() -> AssetData<T> override
        {
            try {
                return AssetDataProxy(path).as<AssetData<T>>();
            }
            catch (const std::bad_variant_access&)
            {
                throw std::runtime_error("[In AssetPathSource::load]: Asset loaded from path "
                                         + path.getFilesystemPath().string() + " is not of the"
                                         " requested type!");
            }
        }

    private:
        AssetPath path;
    };

    template<AssetBaseType T>
    class InMemorySource : public AssetSource<T>
    {
    public:
        explicit InMemorySource(AssetData<T> data)
            : data(std::move(data))
        {}

        auto load() -> AssetData<T> override
        {
            return data;
        }

    private:
        AssetData<T> data;
    };
} // namespace trc
