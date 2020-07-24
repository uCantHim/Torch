#pragma once

#include <vkb/basics/Swapchain.h>

#include "RenderPass.h"
#include "Pipeline.h"

namespace trc::internal
{
    enum RenderPasses : RenderPass::ID
    {
        eDeferredPass = 0,

        NUM_PASSES
    };


    enum DeferredSubPasses
    {
        eGBufferPass = 0,
        eLightingPass = 1,

        NUM_SUBPASSES
    };


    enum Pipelines : GraphicsPipeline::ID
    {
        eDrawableDeferred = 0,
        eDrawableLighting = 1,
    };


    void makeDrawableDeferredPipeline(RenderPass& renderPass,
                                      const DescriptorProviderInterface& generalDescriptorSet);
    void makeFinalLightingPipeline(RenderPass& renderPass,
                                   const DescriptorProviderInterface& generalDescriptorSet,
                                   const DescriptorProviderInterface& gBufferInputSet);
}
