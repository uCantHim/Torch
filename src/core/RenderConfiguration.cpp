#include "trc/core/RenderConfiguration.h"

#include "trc/core/SceneBase.h"



trc::ViewportConfig::ViewportConfig(
    Viewport viewport,
    ResourceStorage resourceStorage,
    std::vector<u_ptr<DrawConfig>> pluginConfigs)
    :
    viewport(viewport),
    resources(std::move(resourceStorage)),
    pluginConfigs(std::move(pluginConfigs))
{
}

void trc::ViewportConfig::update(const Device& device, SceneBase& scene, const Camera& camera)
{
    for (auto& plugin : pluginConfigs) {
        plugin->update(device, scene, camera);
    }
}

void trc::ViewportConfig::createTasks(SceneBase& scene, TaskQueue& taskQueue)
{
    for (auto& plugin : pluginConfigs) {
        plugin->createTasks(scene, taskQueue);
    }
}

auto trc::ViewportConfig::getViewport() const -> Viewport
{
    return viewport;
}

auto trc::ViewportConfig::getResources() -> ResourceStorage&
{
    return resources;
}

auto trc::ViewportConfig::getResources() const -> const ResourceStorage&
{
    return resources;
}



trc::RenderConfig::RenderConfig(const Instance& instance)
    :
    renderGraph{},
    resourceConfig(std::make_shared<ResourceConfig>()),
    pipelineStorage(PipelineRegistry::makeStorage(instance, *resourceConfig))
{
}

void trc::RenderConfig::registerPlugin(s_ptr<RenderPlugin> plugin)
{
    plugin->registerRenderStages(renderGraph);
    plugin->defineResources(*resourceConfig);

    plugins.emplace_back(std::move(plugin));
}

auto trc::RenderConfig::makeViewportConfig(
    const Device& device,
    Viewport viewport)
    -> u_ptr<ViewportConfig>
{
    ResourceStorage resources{ resourceConfig, pipelineStorage };

    std::vector<u_ptr<DrawConfig>> configs;
    for (auto& plugin : plugins)
    {
        auto conf = plugin->createDrawConfig(device, viewport);
        conf->registerResources(resources);

        configs.emplace_back(std::move(conf));
    }

    return std::make_unique<ViewportConfig>(viewport, std::move(resources), std::move(configs));
}

auto trc::RenderConfig::makeViewportConfig(
    const Device& device,
    RenderTarget renderTarget,
    ivec2 renderAreaOffset,
    uvec2 renderArea)
    -> FrameSpecific<u_ptr<ViewportConfig>>
{
    return {
        renderTarget.getFrameClock(),
        [&](ui32 image) {
            Viewport viewport{
                renderTarget.getImage(image),
                renderTarget.getImageView(image),
                renderAreaOffset,
                renderArea
            };
            return makeViewportConfig(device, viewport);
        }
    };
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
    return *resourceConfig;
}

auto trc::RenderConfig::getResourceConfig() const -> const ResourceConfig&
{
    return *resourceConfig;
}
