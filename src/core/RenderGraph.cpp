#include "RenderGraph.h"

#include "RenderPass.h"



void trc::RenderGraph::first(RenderStageType::ID newStage)
{
    insertNewStage(orderedStages.begin(), newStage);
}

void trc::RenderGraph::before(RenderStageType::ID nextStage, RenderStageType::ID newStage)
{
    auto it = std::find_if(
        orderedStages.begin(), orderedStages.end(),
        [nextStage](const auto& pair) {
            return pair.first == nextStage;
        }
    );
    if (it != orderedStages.end()) {
        insertNewStage(it, newStage);
    }
}

void trc::RenderGraph::after(RenderStageType::ID prevStage, RenderStageType::ID newStage)
{
    auto it = std::find_if(
        orderedStages.begin(), orderedStages.end(),
        [prevStage](const auto& pair) {
            return pair.first == prevStage;
        }
    );
    if (it != orderedStages.end()) {
        insertNewStage(it + 1, newStage);
    }
}

bool trc::RenderGraph::contains(RenderStageType::ID stage) const noexcept
{
    return stages.contains(stage);
}

auto trc::RenderGraph::size() const noexcept -> size_t
{
    return orderedStages.size();
}

void trc::RenderGraph::addPass(RenderStageType::ID stage, RenderPass& newPass)
{
    auto it = stages.find(stage);
    if (it != stages.end()) {
        it->second.renderPasses.push_back(&newPass);
    }
}

void trc::RenderGraph::removePass(RenderStageType::ID stage, RenderPass& pass)
{
    auto it = stages.find(stage);
    if (it != stages.end())
    {
        auto& renderPasses = it->second.renderPasses;
        renderPasses.erase(std::remove(renderPasses.begin(), renderPasses.end(), &pass));
    }
}

void trc::RenderGraph::clearPasses(RenderStageType::ID stage)
{
    auto it = stages.find(stage);
    if (it != stages.end()) {
        it->second.renderPasses.clear();
    }
}

void trc::RenderGraph::insertNewStage(auto insertIt, RenderStageType::ID newStage)
{
    auto [it, success] = stages.try_emplace(newStage);
    if (!success) {
        throw std::runtime_error("Duplicate stage in render graph");
    }

    StageInfo& stageStruct = it->second;
    orderedStages.emplace(insertIt, newStage, &stageStruct);
}
