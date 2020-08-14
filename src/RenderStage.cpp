#include "RenderStage.h"



trc::RenderStage::RenderStage(ui32 numSubPasses)
    :
    numSubPasses(numSubPasses)
{
}

auto trc::RenderStage::getRenderPasses() const noexcept -> const std::vector<RenderPass::ID>&
{
    return renderPasses;
}

void trc::RenderStage::addRenderPass(RenderPass::ID newPass)
{
    assert(RenderPass::at(newPass).getNumSubPasses() == numSubPasses);

    if (std::ranges::find(renderPasses, newPass) == renderPasses.end()) {
        renderPasses.push_back(newPass);
    }
}

void trc::RenderStage::removeRenderPass(RenderPass::ID pass)
{
    const auto it = std::ranges::find(renderPasses, pass);
    if (it != renderPasses.end()) {
        renderPasses.erase(it);
    }
}

auto trc::RenderStage::getNumSubPasses() const noexcept -> ui32
{
    return numSubPasses;
}
