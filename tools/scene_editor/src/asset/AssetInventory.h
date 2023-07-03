#pragma once

#include <optional>
#include <shared_mutex>
#include <vector>

#include <trc/assets/AssetManager.h>
#include <trc/assets/AssetPath.h>
#include <trc/assets/Geometry.h>
#include <trc_util/algorithm/IteratorRange.h>
#include <trc_util/data/DeferredInsertVector.h>
#include <trc_util/data/SafeVector.h>

#include "HitboxAsset.h"

/**
 * @brief Manages a physical asset storage directory
 */
class AssetInventory
{
public:
    using const_iterator = trc::AssetManager::const_iterator;

    AssetInventory(trc::AssetManager& man, trc::AssetStorage& storage);

    /**
     * @brief Register all assets in the AssetStorage at the AssetManager
     */
    void detectAssetsFromStorage();

    auto manager() -> trc::AssetManager&;

    template<trc::AssetBaseType T>
    auto import(const trc::AssetPath& dstPath, const trc::AssetData<T>& data)
        -> std::optional<trc::TypedAssetID<T>>;

    void erase(const trc::AssetPath& path);

    template<trc::AssetTraitT Trait>
    auto getTrait(const trc::AssetPath& path) -> Trait*;

    template<
        trc::AssetBaseType Asset,
        std::invocable<AssetInventory&, const trc::AssetPath&, const trc::AssetData<Asset>&> Func
    >
    void registerImportProcessor(Func&& proc);

    auto begin() const -> const_iterator;
    auto end() const -> const_iterator;

private:
        trc::AssetManager* assetManager;
    trc::AssetStorage* persistentStorage;

    using PreprocessFunction = std::function<void(const trc::AssetPath&,
                                                  void* dataPtr)>;

    trc::AssetTypeMap<PreprocessFunction> map;
};



template<trc::AssetBaseType T>
auto AssetInventory::import(const trc::AssetPath& dstPath, const trc::AssetData<T>& _data)
    -> std::optional<trc::TypedAssetID<T>>
{
    // Create a local copy of the asset data
    auto data = _data;

    // Invoke potential data-pre-processing functions
    try {
        map.at<T>()(dstPath, &data);
    }
    catch (const std::out_of_range&) {}

    // Store the asset in the storage and register it at the asset manager
    if (!persistentStorage->store(dstPath, data)) {
        return std::nullopt;
    }

    return assetManager->create<T>(dstPath);
}

template<
    trc::AssetBaseType Asset,
    std::invocable<AssetInventory&, const trc::AssetPath&, const trc::AssetData<Asset>&> Func
>
void AssetInventory::registerImportProcessor(Func&& proc)
{
    auto [_, success] = map.try_emplace<Asset>(
        [this, func=std::forward<Func>(proc)](const trc::AssetPath& path, void* dataPtr) {
            func(*this, path, *static_cast<trc::AssetData<Asset>*>(dataPtr));
        }
    );

    if (!success)
    {
        throw std::out_of_range(
            "[In " + std::string(std::source_location::current().function_name()) + ": "
            "An import processor is already registered for asset type "
            + trc::AssetType::make<Asset>().getName() + " - an asset type may only have a single "
            " import processor defined.");
    }
}
