#pragma once

#include <filesystem>
#include <iostream>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

#include <trc/Types.h>

#include "DynamicAssetTypes.h"

namespace fs = std::filesystem;
using namespace trc::basic_types;

/**
 * Persistent index that tracks asset files and the type of asset that
 * they contain.
 */
class AssetFileIndex
{
public:
    AssetFileIndex() = default;

    void load(const fs::path& indexFile);
    void load(std::istream& is);
    void save(const fs::path& indexFile);
    void save(std::ostream& os);

    /**
     * @return bool True if the path is registered at the index, false
     *              otherwise.
     */
    bool contains(const trc::AssetPath& path) const;

    /**
     * @return AssetType if a file at `path` exitsts. Nothing otherwise.
     */
    auto getType(const trc::AssetPath& path) const -> std::optional<AssetType>;

    /**
     * @brief Iterate over all asset files in the index
     *
     * @param Visitor vis A visitor to resolve asset types dynamically.
     *        Must be a template `<typename T> void(const trc::AssetPath&)`.
     */
    template<typename Visitor>
    void foreach(Visitor&& vis);

    /**
     * @brief Add an asset to the index
     */
    template<trc::AssetBaseType T>
    bool insert(const trc::AssetPath& path);

    /**
     * @brief Remove an asset from the index
     */
    void erase(const trc::AssetPath& path);

public:
    struct AssetInfo
    {
        AssetType type;
    };

private:
    mutable std::shared_mutex indexLock;
    std::unordered_map<trc::AssetPath, AssetInfo> index;
};

template<typename Visitor>
inline void AssetFileIndex::foreach(Visitor&& vis)
{
    std::shared_lock lock(indexLock);
    for (const auto& [path, info] : index)
    {
        fromDynamicType(vis, info.type, path);
    }
}

template<trc::AssetBaseType T>
inline bool AssetFileIndex::insert(const trc::AssetPath& path)
{
    std::scoped_lock lock(indexLock);
    auto [_, success] = index.try_emplace(path, AssetInfo{ .type=toDynamicType<T>() });
    return success;
}
