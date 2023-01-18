#pragma once

#include <atomic>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>

#include "trc/Types.h"
#include "trc/assets/AssetBase.h"
#include "trc/assets/AssetPath.h"
#include "trc/base/Logging.h"

namespace trc
{
    template<AssetBaseType T>
    class AssetSource
    {
    public:
        virtual ~AssetSource() = default;
        virtual auto load() -> AssetData<T> = 0;

        virtual auto getAssetName() -> std::string = 0;
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
            if (!fs::is_regular_file(path.getFilesystemPath()))
            {
                log::error << "Unable to load asset data from " << path.getFilesystemPath()
                           << ": File does not exist.\n";
                return {};
            }

            std::fstream file(path.getFilesystemPath());
            if (!file.is_open())
            {
                log::error << "Unable to load asset data from " << path.getFilesystemPath()
                           << ": Unable to open file (" << file.exceptions() << ")\n";
                return {};
            }

            AssetData<T> data;
            data.deserialize(file);
            return data;
        }

        auto getAssetName() -> std::string override {
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

        auto getAssetName() -> std::string override {
            return name;
        }

    private:
        static inline std::atomic<ui32> uniqueNameIndex{ 0 };

        auto generateName() -> std::string
        {
            std::stringstream ss;
            ss.fill('0');
            ss << "generated_asset_name_" << std::setw(5) << ++uniqueNameIndex;
            return ss.str();
        }

        const AssetData<T> data;
        const std::string name{ generateName() };
    };
} // namespace trc
