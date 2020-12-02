#include "Torch.h"



auto trc::init(const TorchInitInfo& info) -> std::unique_ptr<Renderer>
{
    vkb::vulkanInit();

    return std::make_unique<Renderer>(info.rendererInfo);
}

void trc::terminate()
{
    vkb::getDevice()->waitIdle();

    AssetRegistry::reset();
    RenderPass::destroyAll();
    Pipeline::destroyAll();
    RenderStageType::destroyAll();
    vkb::vulkanTerminate();
}
