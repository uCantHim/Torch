#pragma once

#include <any>

#include "AssetBase.h"
#include "AssetPath.h"

namespace trc
{
    struct AssetSourceLoadHelper
    {
        struct TypeProxy
        {
            template<typename T>
            auto as() -> T {
                return std::any_cast<T>(data);
            }

            std::any data;
        };

        /**
         * Used to hide the AssetDataProxy implementation to prevent circular includes
         */
        static auto loadAsset(const AssetPath& path) -> TypeProxy;
    };

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
                return AssetSourceLoadHelper::loadAsset(path).as<AssetData<T>>();
            }
            catch (const std::bad_any_cast&)
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
