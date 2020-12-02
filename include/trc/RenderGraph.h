#pragma once

#include <vector>
#include <unordered_map>

#include "Types.h"
#include "RenderStage.h"
#include "RenderPass.h"

namespace trc
{
    class RenderGraph
    {
    public:
        void first(RenderStageType::ID newStage);
        void before(RenderStageType::ID nextStage, RenderStageType::ID newStage);
        void after(RenderStageType::ID prevStage, RenderStageType::ID newStage);

        // TODO: Implement this
        // void parallel(RenderStageType::ID parentStage, RenderStageType::ID newStage);

        bool contains(RenderStageType::ID stage) const noexcept;

        void addPass(RenderStageType::ID stage, RenderPass::ID newPass);
        void removePass(RenderStageType::ID stage, RenderPass::ID pass);

        template<typename F>
            requires std::invocable<F, RenderStageType::ID, std::vector<RenderPass::ID>>
        void foreachStage(F func) const
        {
            for (const auto& [stage, info] : orderedStages) {
                func(stage, info->renderPasses);
            }
        }

    private:
        struct StageInfo
        {
            std::vector<RenderPass::ID> renderPasses;
        };

        void insertNewStage(auto it, RenderStageType::ID newStage);

        // Associates a type of stage with an actual stage instance
        std::unordered_map<RenderStageType::ID, StageInfo> stages;

        // Declares ordering of stages
        std::vector<std::pair<RenderStageType::ID, StageInfo*>> orderedStages;
    };
} // namespace trc
