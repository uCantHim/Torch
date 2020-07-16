#pragma once

#include <functional>

#include "Renderpass.h"
#include "Pipeline.h"
#include "SceneRegisterable.h"

namespace trc
{
    template<GraphicsPipeline::ID Pipeline>
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
    template<typename Derived, SubPass::ID SubPass, GraphicsPipeline::ID Pipeline>
    class StaticPipelineRenderInterface
    {
    public:
        StaticPipelineRenderInterface();
    };


    /**
     * @brief A better name
     */
    template<typename Derived, SubPass::ID SubPass, GraphicsPipeline::ID Pipeline>
    using UsePipeline = StaticPipelineRenderInterface<Derived, SubPass, Pipeline>;


#include "DrawableStatic.inl"

} // namespace trc
