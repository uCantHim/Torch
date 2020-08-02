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
        eDrawableDeferredAnimated = 1,
        eDrawableDeferredPickable = 2,
        eDrawableDeferredAnimatedAndPickable = 3,
        eFinalLighting = 4,
        eDrawableInstancedDeferred = 5,
    };


    enum DrawablePipelineFeatureFlagBits : ui32
    {
        eNone = 0,
        eAnimated = 1 << 0,
        ePickable = 1 << 1,
    };

    void makeAllDrawablePipelines(
        RenderPass& renderPass,
        const DescriptorProviderInterface& generalDescriptorSet);

    void makeDrawableDeferredPipeline(
        RenderPass& renderPass,
        const DescriptorProviderInterface& generalDescriptorSet);

    void makeDrawableDeferredAnimatedPipeline(
        RenderPass& renderPass,
        const DescriptorProviderInterface& generalDescriptorSet);

    void makeDrawableDeferredPickablePipeline(
        RenderPass& renderPass,
        const DescriptorProviderInterface& generalDescriptorSet);

    void makeDrawableDeferredAnimatedAndPickablePipeline(
        RenderPass& renderPass,
        const DescriptorProviderInterface& generalDescriptorSet);

    void _makeDrawableDeferredPipeline(
        ui32 pipelineIndex,
        RenderPass& renderPass,
        const DescriptorProviderInterface& generalDescriptorSet,
        ui32 featureFlags
    );


    void makeInstancedDrawableDeferredPipeline(
        RenderPass& renderPass,
        const DescriptorProviderInterface& generalDescriptorSet);

    void makeFinalLightingPipeline(
        RenderPass& renderPass,
        const DescriptorProviderInterface& generalDescriptorSet,
        const DescriptorProviderInterface& gBufferInputSet);
}
