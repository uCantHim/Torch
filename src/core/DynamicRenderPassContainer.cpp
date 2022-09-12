#include "core/DynamicRenderPassContainer.h"

#include <cassert>
#include <ranges>



void trc::DynamicRenderPassContainer::addRenderPass(RenderStage::ID stage, RenderPass& pass)
{
    auto [it, _] = dynamicPasses.try_emplace(stage);
    it->second.emplace_back(&pass);
}

void trc::DynamicRenderPassContainer::removeRenderPass(RenderStage::ID stage, RenderPass& pass)
{
    auto it = dynamicPasses.find(stage);
    if (it != dynamicPasses.end())
    {
        auto& vec = it->second;
        auto end = std::remove_if(vec.begin(), vec.end(), [&](auto el) { return el == &pass; });
        if (end != vec.end()) {
            vec.erase(end, vec.end());
        }
    }
}

void trc::DynamicRenderPassContainer::clearDynamicRenderPasses()
{
    dynamicPasses.clear();
}

void trc::DynamicRenderPassContainer::clearDynamicRenderPasses(RenderStage::ID stage)
{
    auto it = dynamicPasses.find(stage);
    if (it != dynamicPasses.end()) {
        it->second.clear();
    }
}

auto trc::DynamicRenderPassContainer::getDynamicRenderPasses(RenderStage::ID stage)
    -> const std::vector<RenderPass*>&
{
    if (!dynamicPasses.contains(stage)) {
        dynamicPasses.emplace(stage, std::vector<RenderPass*>{});
    }
    return dynamicPasses.at(stage);
}
