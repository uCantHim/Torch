#pragma once

#include "trc/core/RenderStage.h"
#include "trc/core/Task.h"

namespace trc
{
    class RenderPass;

    /**
     * @brief A task that draws classical drawables registered at a
     *        RasterSceneBase for a specific render stage within a specific
     *        instance of a render pass
     */
    class RenderPassDrawTask : public Task
    {
    public:
        RenderPassDrawTask(RenderStage::ID renderStage, s_ptr<RenderPass> renderPass);

        void record(vk::CommandBuffer cmdBuf, TaskEnvironment& env) override;

    private:
        RenderStage::ID renderStage;
        s_ptr<RenderPass> renderPass;
    };
} // namespace trc
