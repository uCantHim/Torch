#include "trc/core/RenderGraph.h"

#include "trc/core/RenderPass.h"



void trc::RenderGraph::first(RenderStage::ID newStage)
{
    insert(stages.begin(), newStage);
}

void trc::RenderGraph::before(RenderStage::ID nextStage, RenderStage::ID newStage)
{
    if (auto stage = findStage(nextStage)) {
        insert(stage.value(), newStage);
    }
}

void trc::RenderGraph::after(RenderStage::ID prevStage, RenderStage::ID newStage)
{
    if (auto stage = findStage(prevStage)) {
        insert(++stage.value(), newStage);
    }
}

void trc::RenderGraph::require(RenderStage::ID stage, RenderStage::ID requiredStage)
{
    if (auto it = findStage(stage)) {
        it.value()->waitDependencies.emplace_back(requiredStage);
    }
}

bool trc::RenderGraph::contains(RenderStage::ID stage) const noexcept
{
    return findStage(stage).has_value();
}

auto trc::RenderGraph::size() const noexcept -> size_t
{
    return stages.size();
}

void trc::RenderGraph::addPass(RenderStage::ID stage, RenderPass& newPass)
{
    auto it = findStage(stage);
    if (it.has_value())
    {
        it.value()->renderPasses.push_back(&newPass);
    }
}

void trc::RenderGraph::removePass(RenderStage::ID stage, RenderPass& pass)
{
    if (auto it = findStage(stage))
    {
        auto& renderPasses = it.value()->renderPasses;

        auto removed = std::remove(renderPasses.begin(), renderPasses.end(), &pass);
        if (removed != renderPasses.end()) {
            renderPasses.erase(removed);
        }
    }
}

auto trc::RenderGraph::findStage(RenderStage::ID stage) -> std::optional<StageIterator>
{
    for (auto it = stages.begin(); it !=stages.end(); ++it)
    {
        if (it->stage == stage) {
            return it;
        }
    }

    return std::nullopt;
}

auto trc::RenderGraph::findStage(RenderStage::ID stage) const -> std::optional<StageConstIterator>
{
    for (auto it = stages.begin(); it !=stages.end(); ++it)
    {
        if (it->stage == stage) {
            return it;
        }
    }

    return std::nullopt;
}

void trc::RenderGraph::insert(StageIterator next, RenderStage::ID newStage)
{
    assert(!contains(newStage));
    stages.insert(next, StageInfo{ .stage=newStage, .renderPasses={}, .waitDependencies={} });
}
