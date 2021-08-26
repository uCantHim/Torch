#pragma once

#include <vector>
#include <unordered_map>

#include "RenderStage.h"
#include "RenderPass.h"

namespace trc
{
    /**
     * @brief A collection of additional scene-specific render passes
     *
     * Scenes can declare additional render passes for certain render
     * stages that are not executed renderer-wide but only for this
     * scene.
     *
     * The render stage, however, must be present in the render graph.
     * Otherwise none of its passes will be executed.
     */
    class DynamicRenderPassContainer
    {
    public:
        DynamicRenderPassContainer() = default;

        /**
         * @brief Add a scene-specific render pass
         */
        void addRenderPass(RenderStageType::ID stage, RenderPass& pass);

        /**
         * @brief Remove a render pass from a stage
         */
        void removeRenderPass(RenderStageType::ID stage, RenderPass& pass);

        /**
         * @brief Remove all dynamic passes for all stages
         */
        void clearDynamicRenderPasses();

        /**
         * @brief Remove all dynamic passes for a specific stage
         *
         * @param RenderStageType::ID stage
         */
        void clearDynamicRenderPasses(RenderStageType::ID stage);

        auto getDynamicRenderPasses(RenderStageType::ID stage) -> const std::vector<RenderPass*>&;

    private:
        std::unordered_map<RenderStageType::ID, std::vector<RenderPass*>> dynamicPasses;
    };
} // namespace trc
