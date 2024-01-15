#include "trc/core/RenderConfiguration.h"



trc::RenderConfig::RenderConfig(RenderGraph layout)
    :
    renderGraph(std::move(layout))
{
}

auto trc::RenderConfig::getRenderGraph() -> RenderGraph&
{
    return renderGraph;
}

auto trc::RenderConfig::getRenderGraph() const -> const RenderGraph&
{
    return renderGraph;
}
