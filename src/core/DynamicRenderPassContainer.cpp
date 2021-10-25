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
    if (it != dynamicPasses.end()) {
        std::ranges::remove_if(it->second, [&](auto el) { return el == &pass; });
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
    try {
        return dynamicPasses.at(stage);
    }
    catch (const std::out_of_range& err) {
        return dynamicPasses.emplace(stage, std::vector<RenderPass*>{}).first->second;
    }
}
