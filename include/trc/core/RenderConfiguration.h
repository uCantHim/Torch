#pragma once

#include <vector>

#include "trc/base/FrameSpecificObject.h"
#include "trc/core/Pipeline.h"
#include "trc/core/RenderGraph.h"
#include "trc/core/RenderPlugin.h"
#include "trc/core/RenderTarget.h"
#include "trc/core/ResourceConfig.h"

namespace trc
{
    class Instance;

    /**
     * @brief A configuration of an entire render cycle
     */
    class RenderConfig
    {
    public:
        RenderConfig(const Instance& instance, RenderTarget target);

        auto getRenderGraph() -> RenderGraph&;
        auto getRenderGraph() const -> const RenderGraph&;

        auto getResourceConfig() -> ResourceConfig&;
        auto getResourceConfig() const -> const ResourceConfig&;
        auto getResourceStorage() -> ResourceStorage&;
        auto getResourceStorage() const -> const ResourceStorage&;

        void setRenderTarget(RenderTarget newTarget,
                             ivec2 renderAreaOffset,
                             uvec2 renderArea);

    protected:
        struct PerFrame
        {
            std::vector<u_ptr<DrawConfig>> pluginDrawConfigs;
            ResourceStorage resources;
        };

        RenderTarget renderTarget;
        RenderGraph renderGraph;
        ResourceConfig resourceConfig;
        s_ptr<PipelineStorage> pipelineStorage;

        std::vector<s_ptr<RenderPlugin>> plugins;

        FrameSpecific<PerFrame> perFrameResources;
    };
} // namespace trc
