#pragma once

#include <atomic>
#include <filesystem>
#include <iomanip>
#include <string>
#include <sstream>

#include "AssetBase.h"
#include "AssetPath.h"
#include "Types.h"

namespace trc
{
    namespace fs = std::filesystem;

    template<AssetBaseType T>
    class AssetSource
    {
    public:
        virtual ~AssetSource() = default;
        virtual auto load() -> AssetData<T> = 0;

        virtual auto getUniqueAssetName() -> std::string = 0;
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

        auto load() -> AssetData<T> override {
            return loadAssetData<T>(path.getFilesystemPath());
        }

        auto getUniqueAssetName() -> std::string override {
            return path.getUniquePath();
        }

    private:
        const AssetPath path;
    };

    template<AssetBaseType T>
    class InMemorySource : public AssetSource<T>
    {
    public:
        explicit InMemorySource(AssetData<T> data)
            : data(std::move(data))
        {}

        auto load() -> AssetData<T> override {
            return data;
        }

        auto getUniqueAssetName() -> std::string override {
            return name;
        }

    private:
        static inline std::atomic<ui32> uniqueNameIndex{ 0 };

        auto generateUniqueName() -> std::string
        {
            std::stringstream ss;
            ss << "generated_asset_name_" << std::setw(5) << ++uniqueNameIndex;
            return ss.str();
        }

        const AssetData<T> data;
        const std::string name{ generateUniqueName() };
    };
} // namespace trc
