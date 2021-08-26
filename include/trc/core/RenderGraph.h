#pragma once

#include <vector>
#include <unordered_map>

#include "Types.h"
#include "RenderStage.h"

namespace trc
{
    class RenderPass;

    class RenderGraph
    {
    public:
        void first(RenderStageType::ID newStage);
        void before(RenderStageType::ID nextStage, RenderStageType::ID newStage);
        void after(RenderStageType::ID prevStage, RenderStageType::ID newStage);

        // TODO: Implement this
        // void parallel(RenderStageType::ID parentStage, RenderStageType::ID newStage);

        /**
         * @param RenderStageType::ID stage
         *
         * @return bool True if the graph contains the specified stage type
         *              at least once.
         */
        bool contains(RenderStageType::ID stage) const noexcept;

        /**
         * @return size_t The number of stages in the graph
         */
        auto size() const noexcept -> size_t;

        void addPass(RenderStageType::ID stage, RenderPass& newPass);
        void removePass(RenderStageType::ID stage, RenderPass& pass);
        void clearPasses(RenderStageType::ID stage);

        template<typename F>
            requires std::invocable<F, RenderStageType::ID, std::vector<RenderPass*>>
        void foreachStage(F func) const
        {
            for (const auto& [stage, info] : orderedStages) {
                func(stage, info->renderPasses);
            }
        }

    private:
        struct StageInfo
        {
            std::vector<RenderPass*> renderPasses;
        };

        void insertNewStage(auto it, RenderStageType::ID newStage);

        // Associates a type of stage with an actual stage instance
        std::unordered_map<RenderStageType::ID, StageInfo> stages;

        // Declares ordering of stages
        std::vector<std::pair<RenderStageType::ID, StageInfo*>> orderedStages;
    };
} // namespace trc
