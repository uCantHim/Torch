#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>

#include "trc/core/RenderStage.h"
#include "trc/core/RenderPass.h"

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
        void addRenderPass(RenderStage::ID stage, RenderPass& pass);

        /**
         * @brief Remove a render pass from a stage
         */
        void removeRenderPass(RenderStage::ID stage, RenderPass& pass);

        /**
         * @brief Remove all dynamic passes for all stages
         */
        void clearDynamicRenderPasses();

        /**
         * @brief Remove all dynamic passes for a specific stage
         *
         * @param RenderStage::ID stage
         */
        void clearDynamicRenderPasses(RenderStage::ID stage);

        auto getDynamicRenderPasses(RenderStage::ID stage) const -> std::vector<RenderPass*>;

    private:
        mutable std::mutex mutex;
        std::unordered_map<RenderStage::ID, std::vector<RenderPass*>> dynamicPasses;
    };
} // namespace trc
