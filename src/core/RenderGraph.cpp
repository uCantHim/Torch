#include "RenderGraph.h"

#include "RenderPass.h"



void trc::RenderGraph::first(RenderStageType::ID newStage)
{
    insert(stages.begin(), newStage);
}

void trc::RenderGraph::before(RenderStageType::ID nextStage, RenderStageType::ID newStage)
{
    auto stage = findStage(nextStage);
    if (stage.has_value()) {
        insert(stage.value(), newStage);
    }
}

void trc::RenderGraph::after(RenderStageType::ID prevStage, RenderStageType::ID newStage)
{
    auto stage = findStage(prevStage);
    if (stage.has_value()) {
        insert(++stage.value(), newStage);
    }
}

void trc::RenderGraph::require(RenderStageType::ID stage, RenderStageType::ID requiredStage)
{
    auto it = findStage(stage);
    if (it.has_value()) {
        it.value()->waitDependencies.emplace_back(requiredStage);
    }
}

bool trc::RenderGraph::contains(RenderStageType::ID stage) const noexcept
{
    return findStage(stage).has_value();
}

auto trc::RenderGraph::size() const noexcept -> size_t
{
    return stages.size();
}

void trc::RenderGraph::addPass(RenderStageType::ID stage, RenderPass& newPass)
{
    auto it = findStage(stage);
    if (it.has_value())
    {
        it.value()->renderPasses.push_back(&newPass);
    }
}

void trc::RenderGraph::removePass(RenderStageType::ID stage, RenderPass& pass)
{
    auto it = findStage(stage);
    if (it.has_value())
    {
        auto& renderPasses = it.value()->renderPasses;
        renderPasses.erase(std::remove(renderPasses.begin(), renderPasses.end(), &pass));
    }
}

auto trc::RenderGraph::compile(const Window& window) const -> RenderLayout
{
    return RenderLayout(window, *this);
}

auto trc::RenderGraph::findStage(RenderStageType::ID stage) -> std::optional<StageIterator>
{
    for (auto it = stages.begin(); it !=stages.end(); ++it)
    {
        if (it->stage == stage) {
            return it;
        }
    }

    return std::nullopt;
}

auto trc::RenderGraph::findStage(RenderStageType::ID stage) const -> std::optional<StageConstIterator>
{
    for (auto it = stages.begin(); it !=stages.end(); ++it)
    {
        if (it->stage == stage) {
            return it;
        }
    }

    return std::nullopt;
}

void trc::RenderGraph::insert(StageIterator next, RenderStageType::ID newStage)
{
    assert(!contains(newStage));
    stages.insert(next, StageInfo{ .stage=newStage, .renderPasses={}, .waitDependencies={} });
}
