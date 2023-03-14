#include "trc/assets/AssetRegistry.h"



void trc::AssetRegistry::AssetModuleUpdatePass::update(
    vk::CommandBuffer cmdBuf,
    FrameRenderState& frameState)
{
    registry->update(cmdBuf, frameState);
}



auto trc::AssetRegistry::getUpdatePass() -> UpdatePass&
{
    return *updateRenderPass;
}

void trc::AssetRegistry::update(vk::CommandBuffer cmdBuf, FrameRenderState& frameState)
{
    modules.update(cmdBuf, frameState);
}
