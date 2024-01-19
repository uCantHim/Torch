#pragma once

#include "trc/core/DescriptorRegistry.h"
#include "trc/core/Pipeline.h"
#include "trc/core/RenderGraph.h"
#include "trc/core/RenderPassRegistry.h"

namespace trc
{
    class ResourceConfig : public RenderPassRegistry
                         , public DescriptorRegistry
    {
    };

    /**
     * @brief A configuration of an entire render cycle
     */
    class RenderConfig : public ResourceConfig
    {
    public:
        explicit RenderConfig(RenderGraph graph);
        virtual ~RenderConfig() = default;

        virtual auto getPipeline(Pipeline::ID id) -> Pipeline& = 0;

        auto getRenderGraph() -> RenderGraph&;
        auto getRenderGraph() const -> const RenderGraph&;

    protected:
        RenderGraph renderGraph;
    };
} // namespace trc
