#include "Torch.h"



auto trc::init(const TorchInitInfo& info) -> std::unique_ptr<Renderer>
{
    vkb::vulkanInit();

    RenderStageType::create(RenderStageTypes::eDeferred, RenderPassDeferred::NUM_SUBPASSES);
    RenderStageType::create(RenderStageTypes::eShadow, 1);

    return std::make_unique<Renderer>(info.rendererInfo);
}

void trc::terminate()
{
    vkb::getDevice()->waitIdle();

    AssetRegistry::reset();
    RenderPass::destroyAll();
    Pipeline::destroyAll();
    RenderStageType::destroy(RenderStageTypes::eDeferred);
    RenderStageType::destroy(RenderStageTypes::eShadow);
    vkb::vulkanTerminate();
}
