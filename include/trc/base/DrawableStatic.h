#pragma once

#include <functional>

#include "RenderPass.h"
#include "Pipeline.h"
#include "SceneRegisterable.h"

namespace trc
{
    template<GraphicsPipeline::ID::Type Pipeline>
    struct PipelineIndex {};

    /**
     * @brief An *optional* static interface for Drawable class specification
     *
     * Use this if you wish to implement static drawable classes, i.e. specific
     * classes that each have distinct drawing capabilities for specific
     * pipelines that are known at compile time.
     *
     * Expects an interface based on overloading via the `PipelineIndex` struct.
     *
     * TODO: Extend this documentation
     * TODO: Add a similar mechanism to declare subpasses at compile time
     */
    template<typename Derived, RenderStage::ID::Type, SubPass::ID::Type, GraphicsPipeline::ID::Type>
    class StaticPipelineRenderInterface
    {
    public:
        StaticPipelineRenderInterface();
    };


    /**
     * @brief A better name
     */
    template<
        typename Derived,
        RenderStage::ID::Type RenderStage,
        SubPass::ID::Type SubPass,
        GraphicsPipeline::ID::Type Pipeline
    >
    using UsePipeline = StaticPipelineRenderInterface<Derived, RenderStage, SubPass, Pipeline>;


#include "DrawableStatic.inl"

} // namespace trc
