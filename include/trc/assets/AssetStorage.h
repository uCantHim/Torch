#pragma once

#include <optional>
#include <utility>

#include <trc_util/Exception.h>

#include "trc/serial/asset.pb.h"
#include "trc/Types.h"
#include "trc/assets/AssetPath.h"
#include "trc/assets/AssetSource.h"
#include "trc/assets/AssetType.h"
#include "trc/assets/Serializer.h"
#include "trc/base/Logging.h"
#include "trc/util/DataStorage.h"
#include "trc/util/Pathlet.h"

namespace trc
{
    class AssetLoadError : Exception
    {
    public:
        AssetLoadError(const AssetPath& path, const std::string& reason)
            : Exception("Unable to load asset from \"" + path.string() + "\": "
                        + reason + ".")
        {}
    };

    /**
     * @brief
     */
    class AssetStorage
    {
    public:
        explicit AssetStorage(s_ptr<DataStorage> storage);

        auto getMetadata(const AssetPath& path) -> std::optional<AssetMetadata>;

        template<AssetBaseType T>
        auto load(const AssetPath& path) -> AssetParseResult<T>;

        /**
         * @brief Create an asset source that can load an asset at a later
         *        time.
         *
         * The created asset source must outlive the AssetStorage by which
         * it was created.
         *
         * Temporarily loads the metadata at `path` into memory to check whether
         * asset exists and it has the requested type.
         *
         * @return optional<u_ptr<AssetSource<T>>> A source object that allows
         *         loading the asset at `path` at a later time.
         *         Returns nullopt if the storage does not contain an asset at
         *         `path` or if the asset at `path` is not of type `T`.
         */
        template<AssetBaseType T>
        auto loadDeferred(const AssetPath& path)
            -> std::expected<u_ptr<AssetSource<T>>, AssetParseError>;

        template<AssetBaseType T>
        bool store(const AssetPath& path, const AssetData<T>& data);

        /**
         * @brief Delete an item from storage.
         *
         * Note that this may be a permanent action, depending on the underlying
         * data storage implementation. For example, a filesystem storage may
         * delete the respective data file.
         */
        bool remove(const AssetPath& path);

        struct AssetIterator
        {
            using const_reference = const AssetPath&;
            using const_pointer = const AssetPath*;

            AssetIterator(DataStorage::iterator begin,
                          DataStorage::iterator end,
                          s_ptr<DataStorage> storage);

            auto operator*() const -> const_reference;
            auto operator->() const -> const_pointer;

            auto operator++() -> AssetIterator&;

            bool operator==(const AssetIterator& other) const;
            bool operator!=(const AssetIterator& other) const = default;

        private:
            bool isAssetFile(const util::Pathlet& path);
            void step();

            DataStorage::iterator iter;
            DataStorage::iterator end;
            std::optional<AssetPath> currentPath;
            s_ptr<DataStorage> storage;
        };

        using iterator = AssetIterator;

        auto begin() -> iterator;
        auto end() -> iterator;

    private:
        static auto getMetadata(const serial::AssetFile& file) -> AssetMetadata;

        bool isAssetFile(const util::Pathlet& path);
        auto loadFile(const util::Pathlet& path) -> std::expected<serial::AssetFile, std::string>;
        auto writeFile(const util::Pathlet& path, const serial::AssetFile& file)
            -> std::optional<std::string>;

        s_ptr<DataStorage> storage;
    };

    /**
     * @brief Asset source that loads data from an AssetStorage
     */
    template<AssetBaseType T>
    class AssetStorageSource : public AssetSource<T>
    {
    public:
        AssetStorageSource(AssetPath path, const s_ptr<DataStorage>& storage)
            : path(std::move(path)), storage(storage)
        {}

        auto load() -> AssetData<T> override
        {
            auto data = AssetStorage{storage}.load<T>(path);
            if (!data.has_value())
            {
                log::error << log::here() << ": " << data.error().message;
                throw data.error();
            }
            return *data;
        }

        auto getMetadata() -> AssetMetadata override
        {
            auto meta = AssetStorage{storage}.getMetadata(path);
            if (!meta.has_value())
            {
                log::error << "Unable to load asset metadata for " << path.string()
                           << ": file \"" << path.string()
                           << "\" not found in the asset storage.\n";
                throw AssetLoadError(path, "Metadata not found in storage.");
            }
            return std::move(*meta);
        }

    private:
        const AssetPath path;
        s_ptr<DataStorage> storage;
        std::optional<serial::AssetFile> file;
    };

    template<AssetBaseType T>
    auto AssetStorage::load(const AssetPath& path) -> AssetParseResult<T>
    {
        auto file = loadFile(path);
        if (!file) {
            return std::unexpected(AssetParseError{
                .errorCode=AssetParseError::Code::eSystemError,
                .message=file.error(),
            });
        }

        // Ensure that the correct type of asset is stored at `path`
        const auto meta = getMetadata(*file);
        if (meta.type != AssetType::make<T>())
        {
            return std::unexpected(AssetParseError{
                AssetParseError::Code::eSemanticError,
                "Asset at " + path.string() + " is not of the requested type "
                + AssetType::make<T>().getName() + ". (Actual type: " + meta.type.getName() + ")"
            });
        }

        // Load and parse data
        std::stringstream ss{ file->asset_data() };
        return AssetSerializerTraits<T>::deserialize(ss);
    }

    template<AssetBaseType T>
    auto AssetStorage::loadDeferred(const AssetPath& path)
        -> std::expected<u_ptr<AssetSource<T>>, AssetParseError>
    {
        // Ensure that the correct type of asset is stored at `path`
        auto file = loadFile(path);
        if (!file) {
            return std::unexpected(AssetParseError{
                AssetParseError::Code::eSyntaxError,
                file.error()
            });
        }
        if (auto meta = getMetadata(*file); !meta.type.is<T>())
        {
            return std::unexpected(AssetParseError{
                AssetParseError::Code::eSemanticError,
                "Asset at " + path.string() + " is not of the requested type "
                + AssetType::make<T>().getName() + ". (Actual type: " + meta.type.getName() + ")"
            });
        }

        return std::make_unique<AssetStorageSource<T>>(path, this->storage);
    }

    template<AssetBaseType T>
    bool AssetStorage::store(const AssetPath& path, const AssetData<T>& data)
    {
        serial::AssetFile file;

        // Write metadata
        *file.mutable_metadata() = AssetMetadata{
            .name=path.getAssetName(),
            .type=AssetType::make<T>(),
            .path=path,
        }.serialize();

        // Write asset data
        std::stringstream ss;
        AssetSerializerTraits<T>::serialize(data, ss);
        file.set_asset_data(ss.str());

        auto res = writeFile(path, file);
        return !res.has_value();
    }
} // namespace trc
