#pragma once

#include <filesystem>

#include "AssetBase.h"
#include "AssetPath.h"

namespace trc
{
    namespace fs = std::filesystem;

    template<AssetBaseType T>
    class AssetSource
    {
    public:
        virtual ~AssetSource() = default;
        virtual auto load() -> AssetData<T> = 0;
    };

    /**
     * Implement/specialize this template for an asset type to define a
     * load operation for it.
     */
    template<AssetBaseType T>
    auto loadAssetData(const fs::path& path) -> AssetData<T>;

    /**
     * @brief Loads data from a file
     *
     * Uses a `loadAssetData` function that implements the loading and
     * parsing from file.
     */
    template<AssetBaseType T>
        requires requires (AssetPath path) {
            { loadAssetData<T>(path.getFilesystemPath()) } -> std::same_as<AssetData<T>>;
        }
    class AssetPathSource : public AssetSource<T>
    {
    public:
        explicit AssetPathSource(AssetPath path)
            : path(std::move(path))
        {}

        auto load() -> AssetData<T> override
        {
            return loadAssetData<T>(path.getFilesystemPath());
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
