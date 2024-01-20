#include "trc/core/RenderConfiguration.h"



trc::RenderConfig::RenderConfig(const Instance& instance, RenderTarget target)
    :
    renderTarget(std::move(target)),
    renderGraph(),
    resourceConfig(),
    pipelineStorage(PipelineRegistry::makeStorage(instance, resourceConfig)),
    perFrameResources(
        renderTarget.getFrameClock(),
        [&](ui32) {
            return PerFrame{ {}, ResourceStorage{ &resourceConfig, pipelineStorage } };
        }
    )
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

auto trc::RenderConfig::getResourceConfig() -> ResourceConfig&
{
    return resourceConfig;
}

auto trc::RenderConfig::getResourceConfig() const -> const ResourceConfig&
{
    return resourceConfig;
}

auto trc::RenderConfig::getResourceStorage() -> ResourceStorage&
{
    return perFrameResources->resources;
}

auto trc::RenderConfig::getResourceStorage() const -> const ResourceStorage&
{
    return perFrameResources->resources;
}

void trc::RenderConfig::setRenderTarget(RenderTarget newTarget,
                     ivec2 renderAreaOffset,
                     uvec2 renderArea)
{
    // TODO: Create viewport configs from plugins, etc.
}
