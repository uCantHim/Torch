#pragma once

#include "trc/core/RenderStage.h"

/**
 * @brief A collection of all of Torch's built-in render stages.
 */
namespace trc::stages
{
    inline const RenderStage resourceUpdate = RenderStage::make("ResourceUpdate");

    inline const RenderStage gBuffer = RenderStage::make("gBuffer");
    inline const RenderStage shadow = RenderStage::make("ShadowMaps");
    inline const RenderStage deferredLighting = RenderStage::make("DeferredLighting");

    inline const RenderStage rayTracing = RenderStage::make("RayTracing");
    inline const RenderStage rayCompositing = RenderStage::make("RayCompositing");

    /**
     * A stage that occurs before all other render stages.
     */
    inline const RenderStage pre = RenderStage::make("Pre");

    /**
     * A stage that occurs after all other render stages.
     *
     * Custom post-processing can depend on this stage.
     */
    inline const RenderStage post = RenderStage::make("Post");

    /**
     * A stage that initializes the render target image. All of Torch's render
     * plugins that draw to an image depend on this stage.
     *
     * For example, one might transition the image into a valid layout if
     * acquiring it from a presentation engine. `SwapchainPlugin` does this.
     */
    inline const RenderStage renderTargetImageInit = RenderStage::make("RenderTargetInit");

    /**
     * A stage that finalizes the render target image. All of Torch's render
     * plugins that draw to an image depend on this stage.
     *
     * For example, one might transition the image into ePresentSrcKHR if
     * submitting it to a presentation engine. `SwapchainPlugin` does this.
     */
    inline const RenderStage renderTargetImageFinalize = RenderStage::make("RenderTargetFinalize");
} // namespace trc
