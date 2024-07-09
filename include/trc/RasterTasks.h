#pragma once

#include "trc/core/RenderPipelineTasks.h"
#include "trc/core/RenderStage.h"

namespace trc
{
    class RasterSceneModule;
    class RenderPass;

    /**
     * @brief A task that draws classical drawables registered at a
     *        RasterSceneBase for a specific render stage within a specific
     *        instance of a render pass
     */
    class RenderPassDrawTask : public ViewportDrawTask
    {
    public:
        RenderPassDrawTask(RenderStage::ID renderStage,
                           s_ptr<RenderPass> renderPass);

        void record(vk::CommandBuffer cmdBuf, ViewportDrawContext& env) override;

    private:
        RenderStage::ID renderStage;
        s_ptr<RenderPass> renderPass;
    };
} // namespace trc
