#pragma once

#include "trc/core/Task.h"

namespace trc
{
    class RenderPass;

    /**
     * @brief A task that draws classical drawables registered at a
     *        RasterSceneBase for a specific render pass
     */
    class RenderPassDrawTask : public Task
    {
    public:
        explicit RenderPassDrawTask(s_ptr<RenderPass> renderPass);

        void record(vk::CommandBuffer cmdBuf, TaskEnvironment& env) override;

    private:
        s_ptr<RenderPass> renderPass;
    };
} // namespace trc
