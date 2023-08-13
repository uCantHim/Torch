#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "trc/core/DescriptorRegistry.h"
#include "trc/core/Pipeline.h"
#include "trc/core/RenderGraph.h"
#include "trc/core/RenderPass.h"
#include "trc/core/RenderStage.h"

namespace trc
{
    struct DrawConfig;
    class RenderTarget;

    /**
     * @brief Strong type to reference a render pass
     */
    struct RenderPassName
    {
        std::string identifier;
    };

    class RenderPassRegistry
    {
    public:
        struct RenderPassDefinition
        {
            vk::RenderPass pass;
            ui32 subpass;
        };

        using RenderPassGetter = std::function<RenderPassDefinition()>;

        void addRenderPass(RenderPassName name, RenderPassGetter getter);

        auto getRenderPass(const RenderPassName& name) const
            -> RenderPassDefinition;

    private:
        std::unordered_map<std::string, RenderPassGetter> renderPasses;
    };

    /**
     * @brief A configuration of an entire render cycle
     */
    class RenderConfig : public RenderPassRegistry
                       , public DescriptorRegistry
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
