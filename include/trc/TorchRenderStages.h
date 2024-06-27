#pragma once

#include "trc/core/RenderStage.h"

namespace trc
{
    /**
     * A stage that initializes the render target image. All of Torch's render
     * plugins that draw to an image depend on this stage.
     *
     * For example, one might transition the image into a valid layout if
     * acquiring it from a presentation engine. `SwapchainPlugin` does this.
     */
    inline RenderStage renderTargetImageInitStage = RenderStage::make();

    /**
     * A stage that finalizes the render target image. All of Torch's render
     * plugins that draw to an image depend on this stage.
     *
     * For example, one might transition the image into ePresentSrcKHR if
     * submitting it to a presentation engine. `SwapchainPlugin` does this.
     */
    inline RenderStage renderTargetImageFinalizeStage = RenderStage::make();

    inline RenderStage resourceUpdateStage = RenderStage::make();

    inline RenderStage gBufferRenderStage = RenderStage::make();
    inline RenderStage shadowRenderStage = RenderStage::make();
    inline RenderStage finalLightingRenderStage = RenderStage::make();

    inline RenderStage rayTracingRenderStage = RenderStage::make();
    inline RenderStage finalCompositingRenderStage = RenderStage::make();
} // namespace trc
