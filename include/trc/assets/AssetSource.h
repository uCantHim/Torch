#pragma once

#include <atomic>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>

#include "AssetBase.h"
#include "AssetPath.h"
#include "Types.h"

namespace trc
{
    template<AssetBaseType T>
    class AssetSource
    {
    public:
        virtual ~AssetSource() = default;
        virtual auto load() -> AssetData<T> = 0;

        virtual auto getUniqueAssetName() -> std::string = 0;
    };

    /**
     * @brief Loads data from a file
     */
    template<AssetBaseType T>
    class AssetPathSource : public AssetSource<T>
    {
    public:
        explicit AssetPathSource(AssetPath path)
            : path(std::move(path))
        {}

        auto load() -> AssetData<T> override
        {
            std::fstream file(path.getFilesystemPath());
            AssetData<T> data;
            data.deserialize(file);
            return data;
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
