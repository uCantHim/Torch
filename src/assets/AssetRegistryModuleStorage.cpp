#include "trc/assets/AssetRegistryModuleStorage.h"



namespace trc
{

void AssetRegistryModuleStorage::update(vk::CommandBuffer cmdBuf, FrameRenderState& state)
{
    std::scoped_lock lock(entriesLock);
    for (auto& entry : entries)
    {
        assert(entry != nullptr);
        entry->update(cmdBuf, state);
    }
}

} // namespace trc
