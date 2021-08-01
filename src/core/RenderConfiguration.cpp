#include "RenderConfiguration.h"

#include "DrawConfiguration.h"



auto trc::RenderConfig::getGraph() -> RenderGraph&
{
    return graph;
}

auto trc::RenderConfig::getGraph() const -> const RenderGraph&
{
    return graph;
}
