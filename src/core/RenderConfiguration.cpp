#include "trc/core/RenderConfiguration.h"



trc::RenderConfig::RenderConfig(
    const Instance& instance,
    const RenderTarget& renderTarget,
    ivec2 renderOffset,
    uvec2 renderArea)
    :
    resourceConfig(),
    pipelineStorage(PipelineRegistry::makeStorage(instance, resourceConfig)),
    viewports(
        renderTarget.getFrameClock(),
        [&](ui32 i) {
            return Viewport{
                renderTarget.getImage(i),
                renderTarget.getImageView(i),
                renderOffset,
                renderArea
            };
        }
    ),
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

void trc::RenderConfig::setRenderTarget(
    RenderTarget newTarget,
    ivec2 renderAreaOffset,
    uvec2 renderArea)
{
    // TODO: Create viewport configs from plugins, etc.
}
