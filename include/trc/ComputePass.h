#pragma once

#include "RenderPass.h"
#include "Pipeline.h"
#include "base/SceneBase.h"

namespace trc
{
    class ComputeExecution
    {
    public:
        void attachToScene(SceneBase& scene);
        void removeFromScene();
    };

    class ComputePass : public RenderPass
    {
    public:
        // Has one subpass
        ComputePass() : RenderPass({}, 1) {}

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents) override;
        void end(vk::CommandBuffer cmdBuf) override;

        /**
         * A compute shader that is alwas executed in the pass.
         */
        void addStaticExecution(Pipeline::ID pipeline, DrawableFunction executionFunc);

    private:
        std::vector<std::pair<Pipeline::ID, DrawableFunction>> staticExecutions;
    };
}
