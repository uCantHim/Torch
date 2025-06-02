#include "trc/assets/AssetRegistryModuleStorage.h"

#include <ranges>



namespace trc
{

AssetRegistryModuleStorage::~AssetRegistryModuleStorage()
{
    // Delete modules in reverse order of insertion to respect inter-module
    // dependencies (example: the material registry references textures and thus
    // must be destroyed before the texture registry).
    std::scoped_lock lock(entriesLock);
    for (auto& mod : std::views::reverse(entries)) {
        mod.reset();
    }
}

void AssetRegistryModuleStorage::update(vk::CommandBuffer cmdBuf, FrameRenderState& state)
{
    std::scoped_lock lock(entriesLock);
    for (auto& entry : entries | std::views::filter([](auto& e){ return e != nullptr; }))
    {
        entry->update(cmdBuf, state);
    }
}

} // namespace trc
