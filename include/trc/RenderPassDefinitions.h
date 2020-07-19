#pragma once

#include <vkb/basics/Swapchain.h>

#include "Renderpass.h"
#include "Pipeline.h"

namespace trc::internal
{
    enum RenderPasses : SubPass::ID
    {
        eDeferredPass = 0,
        eLightingPass = 1,

        NUM_PASSES
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
