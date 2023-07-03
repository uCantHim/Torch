#include "AssetInventory.h"

#include <trc/base/Logging.h>



AssetInventory::AssetInventory(trc::AssetManager& man, trc::AssetStorage& storage)
    :
    assetManager(&man),
    persistentStorage(&storage)
{
}

void AssetInventory::detectAssetsFromStorage()
{
    // Register all assets from disk at the AssetManager
    for (const trc::AssetPath& path : *persistentStorage)
    {
        auto asset = assetManager->create(path);
        if (!asset)
        {
            trc::log::warn << trc::log::here()
                           << ": Unable to load asset from " << path.string() << ".";
        }
    }
}

auto AssetInventory::manager() -> trc::AssetManager&
{
    return *assetManager;
}

void AssetInventory::erase(const trc::AssetPath& erasedPath)
{
    assetManager->destroy(erasedPath);
    persistentStorage->remove(erasedPath);
}

auto AssetInventory::begin() const -> const_iterator
{
    return assetManager->begin();
}

auto AssetInventory::end() const -> const_iterator
{
    return assetManager->end();
}
