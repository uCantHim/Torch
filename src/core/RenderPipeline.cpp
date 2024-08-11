#include "trc/core/RenderPipeline.h"

#include <cassert>

#include <generator>
#include <ranges>

#include "trc/core/Frame.h"



namespace trc
{

RenderPipelineViewport::RenderPipelineViewport(
    RenderPipeline& pipeline,
    ui32 viewportIndex,
    const RenderArea& renderArea,
    const s_ptr<Camera>& camera,
    const s_ptr<SceneBase>& scene)
    :
    parent(pipeline),
    vpIndex(viewportIndex),
    area(renderArea),
    camera(camera),
    scene(scene)
{
    assert_arg(camera != nullptr);
    assert_arg(scene != nullptr);
}

auto RenderPipelineViewport::getRenderTarget() -> const RenderTarget&
{
    return parent.getRenderTarget();
}

auto RenderPipelineViewport::getRenderArea() -> const RenderArea&
{
    return area;
}

auto RenderPipelineViewport::getCamera() -> Camera&
{
    return *camera;
}

auto RenderPipelineViewport::getScene() -> SceneBase&
{
    return *scene;
}

void RenderPipelineViewport::resize(const RenderArea& newArea)
{
    throw std::logic_error("not implemented");
}

void RenderPipelineViewport::setCamera(s_ptr<Camera> camera)
{
    throw std::logic_error("not implemented");
}

void RenderPipelineViewport::setScene(s_ptr<SceneBase> scene)
{
    throw std::logic_error("not implemented");
}



auto RenderPipelineBuilder::build(const RenderPipelineCreateInfo& createInfo) -> u_ptr<RenderPipeline>
{
    return std::make_unique<RenderPipeline>(
        createInfo.instance,
        pluginBuilders,
        createInfo.renderTarget,
        createInfo.maxViewports
    );
}

auto RenderPipelineBuilder::addPlugin(PluginBuilder builder) -> Self&
{
    pluginBuilders.emplace_back(std::move(builder));
    return *this;
}



RenderPipeline::RenderPipeline(
    const Instance& instance,
    std::span<PluginBuilder> pluginBuilders,
    const RenderTarget& _renderTarget,
    const ui32 maxViewports)
    :
    // Constants
    device(instance.getDevice()),
    maxViewports(maxViewports),

    // Pipeline-defining rendering entities
    renderTarget(_renderTarget),
    renderGraph(std::make_shared<RenderGraph>()),

    // Resource storage
    resourceConfig(std::make_shared<ResourceConfig>()),
    pipelineStorage(PipelineRegistry::makeStorage(instance, *resourceConfig)),
    topLevelResourceStorage(std::make_shared<ResourceStorage>(
        resourceConfig,
        pipelineStorage
    )),

    // Initialize these later
    pipelinesPerFrame(nullptr)
{
    assert(renderGraph != nullptr);
    assert(resourceConfig != nullptr);
    assert(topLevelResourceStorage != nullptr);

    // Create all render plugins.
    PluginBuildContext ctx{ instance, *this };
    for (auto& builder : pluginBuilders)
    {
        auto plugin = builder(ctx);
        if (plugin == nullptr) continue;

        plugin->defineRenderStages(*renderGraph);
        plugin->defineResources(*resourceConfig);
        renderPlugins.emplace_back(std::move(plugin));
    }

    // Initialize rendertarget-dependent stuff now
    initForRenderTarget(_renderTarget);
}

auto RenderPipeline::draw() -> u_ptr<Frame>
{
    auto getAllViewportIndices = [](const PipelineInstance& pipeline)
        -> std::generator<ui32>
    {
        for (ui32 i = 0; auto& vp : pipeline.viewports)
        {
            if (vp != nullptr) co_yield i;
            ++i;
        }
    };

    auto frame = std::make_unique<Frame>(
        device,
        renderGraph->compile(),
        topLevelResourceStorage
    );
    drawToFrame(*frame, getAllViewportIndices(pipelinesPerFrame->get()));

    return frame;
}

auto RenderPipeline::draw(const vk::ArrayProxy<ViewportHandle>& viewports) -> u_ptr<Frame>
{
    auto frame = std::make_unique<Frame>(
        device,
        renderGraph->compile(),
        topLevelResourceStorage
    );

    drawToFrame(
        *frame,
        std::views::transform(viewports, [](auto&& vp) { return vp->vpIndex; })
    );

    return frame;
}

auto RenderPipeline::makeViewport(
    const RenderArea& renderArea,
    const s_ptr<Camera>& camera,
    const s_ptr<SceneBase>& scene) -> ViewportHandle
{
    assert_arg(camera != nullptr);
    assert_arg(scene != nullptr);

    // If the scene is new, create all necessary resources for it.
    useScene(scene);

    // Allocate a slot in the fixed-size vector of viewport resources.
    const ui32 viewportIndex = viewportIdPool.generate();
    if (viewportIndex >= maxViewports)
    {
        // Don't free the new index! We don't want an out-of-bounds index in
        // the pool of available indices.
        throw std::out_of_range("Too many viewports!");
    }

    // Create resources specific to the new viewport
    for (auto [img, pipeline] : std::views::zip(renderTarget.getRenderImages(), *pipelinesPerFrame))
    {
        assert(pipeline.viewports.size() == maxViewports);
        assert(viewportIndex < pipeline.viewports.size());
        assert(pipeline.viewports[viewportIndex] == nullptr);

        pipeline.viewports[viewportIndex] = instantiateViewport(
            pipeline,
            img,
            renderArea,
            camera,
            scene
        );
    }

    return std::shared_ptr<RenderPipelineViewport>{
        new RenderPipelineViewport{ *this, viewportIndex, renderArea, camera, scene },
        [this, viewportIndex](RenderPipelineViewport* hnd)
        {
            this->freeScene(hnd->scene);
            this->freeViewport(viewportIndex);
            delete hnd;
        }
    };
}

auto RenderPipeline::getMaxViewports() const -> ui32
{
    return maxViewports;
}

auto RenderPipeline::getResourceConfig() -> ResourceConfig&
{
    assert(resourceConfig != nullptr);
    return *resourceConfig;
}

auto RenderPipeline::getRenderGraph() -> RenderGraph&
{
    assert(renderGraph != nullptr);
    return *renderGraph;
}

auto RenderPipeline::getRenderTarget() const -> const RenderTarget&
{
    return renderTarget;
}

void RenderPipeline::changeRenderTarget(const RenderTarget& newTarget)
{
    // We keep all allocated viewport handles intact and re-create their
    // backing resources.
    const auto usedViewports = std::ranges::to<std::vector>(getUsedViewports());

    // Destroy all resources (global, scenes, and viewports).
    pipelinesPerFrame.reset();
    uniqueScenes.clear();

    // Re-create global resources for the new render target.
    initForRenderTarget(newTarget);

    // Re-create resources for all viewports and scenes that are currently in use.
    for (const auto& vp : usedViewports)
    {
        useScene(vp->scene);
        for (auto [img, pipeline] : std::views::zip(renderTarget.getRenderImages(),
                                                    *pipelinesPerFrame))
        {
            pipeline.viewports.at(vp->vpIndex) = instantiateViewport(
                pipeline, img,
                vp->getRenderArea(),
                vp->camera,
                vp->scene
            );
        }
    }
}

void RenderPipeline::recordGlobal(Frame& frame, PipelineInstance& pipeline)
{
    auto& global = pipeline.global;

    RenderPipelineContext ctx{ global.info };
    auto& queue = global.taskQueue;
    for (auto& plugin : global.pluginImpls)
    {
        plugin->hostUpdate(ctx);
        plugin->createTasks(queue);
    }

    queue.moveTasks<DeviceExecutionContext>(
        [&global](DeviceExecutionContext& baseCtx) -> GlobalUpdateContext {
            auto ctx = baseCtx.overrideResources(global.resources);
            return { ctx, global.info };
        },
        frame.getTaskQueue()
    );
}

void RenderPipeline::recordScenes(Frame& frame, PipelineInstance& pipeline)
{
    for (auto& [_, scene] : pipeline.scenes)
    {
        assert(scene != nullptr);

        auto& queue = scene->taskQueue;
        SceneContext ctx{ pipeline.global.info, scene->info };
        for (auto& plugin : scene->pluginImpls)
        {
            plugin->hostUpdate(ctx);
            plugin->createTasks(queue);
        }

        queue.moveTasks<DeviceExecutionContext>(
            [&scene](DeviceExecutionContext& baseCtx) -> SceneUpdateContext {
                auto ctx = baseCtx.overrideResources(scene->resources);
                return { ctx, scene->info };
            },
            frame.getTaskQueue()
        );
    }
}

void RenderPipeline::recordViewports(
    Frame& frame,
    PipelineInstance& pipeline,
    std::ranges::range auto&& vpIndices)
{
    for (const auto& i : vpIndices)
    {
        auto& vp = pipeline.viewports.at(i);

        // Skip `nullptr` viewports because the vector is fixed-length and not
        // all slots may be used.
        if (!vp) continue;

        auto& queue = vp->taskQueue;
        ViewportContext ctx{ pipeline.global.info, vp->info };
        for (auto& plugin : vp->pluginImpls)
        {
            plugin->hostUpdate(ctx);
            plugin->createTasks(queue, ctx);
        }

        queue.template moveTasks<DeviceExecutionContext>(
            [&vp](DeviceExecutionContext& baseCtx) -> ViewportDrawContext {
                auto ctx = baseCtx.overrideResources(vp->resources);
                return { ctx, vp->info };
            },
            frame.getTaskQueue()
        );
    }
}

void RenderPipeline::initForRenderTarget(const RenderTarget& target)
{
    renderTarget = target;

    pipelinesPerFrame.reset();
    pipelinesPerFrame = std::make_unique<FrameSpecific<PipelineInstance>>(
        target.getFrameClock(),
        [&](ui32) -> PipelineInstance
        {
            impl::RenderPipelineInfo info{ device, *this };
            auto resourceStorage = ResourceStorage::derive(topLevelResourceStorage);
            return {
                .global{
                    .info{ info },
                    .pluginImpls{},  // Are created after the plugins have been
                                     // created. See constructor body below.
                    .taskQueue{},
                    .resources{ std::move(resourceStorage) },
                },
                .scenes{},     // No scenes or viewports yet.
                .viewports{},  // Create these on demand.
            };
        }
    );

    // Initialize render pipeline instances per frame.
    for (auto& pipeline : *pipelinesPerFrame)
    {
        assert(pipeline.global.resources != nullptr);

        // Create global (viewport-independent) resources.
        pipeline.global.pluginImpls = createPluginGlobalInstances(
            *pipeline.global.resources,
            pipeline.global.info
        );

        // Initialize an empty fixed-size vector for all the viewports that may
        // exist.
        pipeline.viewports.resize(maxViewports);
    }
}

auto RenderPipeline::createPluginGlobalInstances(
    ResourceStorage& resourceStorage,
    const impl::RenderPipelineInfo& pipeline)
    -> std::vector<u_ptr<GlobalResources>>
{
    RenderPipelineContext ctx{ pipeline };

    std::vector<u_ptr<GlobalResources>> pluginResources;
    for (auto& plugin : renderPlugins)
    {
        if (auto res = plugin->createGlobalResources(ctx))
        {
            res->registerResources(resourceStorage);
            pluginResources.emplace_back(std::move(res));
        }
    }

    return pluginResources;
}

auto RenderPipeline::createPluginSceneInstances(
    ResourceStorage& resourceStorage,
    const impl::RenderPipelineInfo& pipeline,
    const s_ptr<SceneBase>& scene)
    -> std::vector<u_ptr<SceneResources>>
{
    SceneContext ctx{ pipeline, scene };

    std::vector<u_ptr<SceneResources>> pluginResources;
    for (auto& plugin : renderPlugins)
    {
        if (auto res = plugin->createSceneResources(ctx))
        {
            res->registerResources(resourceStorage);
            pluginResources.emplace_back(std::move(res));
        }
    }

    return pluginResources;
}

auto RenderPipeline::createPluginViewportInstances(
    ResourceStorage& resourceStorage,
    const impl::RenderPipelineInfo& pipeline,
    const impl::ViewportInfo& viewport)
    -> std::vector<u_ptr<ViewportResources>>
{
    ViewportContext ctx{ pipeline, viewport };

    std::vector<u_ptr<ViewportResources>> pluginResources;
    for (auto& plugin : renderPlugins)
    {
        if (auto res = plugin->createViewportResources(ctx))
        {
            res->registerResources(resourceStorage);
            pluginResources.emplace_back(std::move(res));
        }
    }

    return pluginResources;
}

void RenderPipeline::useScene(const s_ptr<SceneBase>& scene)
{
    if (uniqueScenes.emplace(scene) > 1) {
        return;
    }

    // Create resources specific to the scene
    for (auto& pipeline : *pipelinesPerFrame)
    {
        assert(!pipeline.scenes.contains(scene));

        auto resourceStorage = ResourceStorage::derive(pipeline.global.resources);
        auto pluginInstances = createPluginSceneInstances(*resourceStorage, pipeline.global.info, scene);
        pipeline.scenes.try_emplace(scene, std::make_unique<PerScene>(
            PerScene{
                .info{ scene },
                .pluginImpls{ std::move(pluginInstances) },
                .taskQueue{},
                .resources{ std::move(resourceStorage) },
            }
        ));
    }
}

void RenderPipeline::freeScene(const s_ptr<SceneBase>& scene)
{
    if (uniqueScenes.erase(scene) == 0)
    {
        for (auto& pipeline : *pipelinesPerFrame) {
            pipeline.scenes.erase(scene);
        }
    }
}

auto RenderPipeline::instantiateViewport(
    PipelineInstance& pipeline,
    const RenderImage& img,
    const RenderArea& renderArea,
    const s_ptr<Camera>& camera,
    const s_ptr<SceneBase>& scene)
    -> u_ptr<PerViewport>
{
    assert(camera != nullptr);
    assert(scene != nullptr);

    const impl::ViewportInfo vpInfo{ Viewport{ img, renderArea }, camera, scene };

    auto baseResources = pipeline.scenes.at(scene)->resources;
    auto resourceStorage = ResourceStorage::derive(baseResources);
    auto pluginInstances = createPluginViewportInstances(
        *resourceStorage,
        pipeline.global.info,
        vpInfo
    );
    return std::make_unique<PerViewport>(
        PerViewport{
            .info{ vpInfo },
            .pluginImpls{ std::move(pluginInstances) },
            .taskQueue{},
            .resources{ std::move(resourceStorage) },
        }
    );
}

auto RenderPipeline::getUsedViewports() const -> std::generator<ViewportHandle>
{
    for (auto& handle : allocatedViewports)
    {
        if (auto vp = handle.lock()) {
            co_yield vp;
        }
    }
}

void RenderPipeline::freeViewport(ui32 viewportIndex)
{
    for (auto& pipeline : *pipelinesPerFrame)
    {
        assert(pipeline.viewports.size() > viewportIndex);
        assert(pipeline.viewports[viewportIndex] != nullptr);

        pipeline.viewports[viewportIndex].reset();
    }
    viewportIdPool.free(viewportIndex);
}

void RenderPipeline::drawToFrame(Frame& frame, std::ranges::range auto&& vpIndices)
{
    auto& curPipeline = pipelinesPerFrame->get();
    recordGlobal(frame, curPipeline);
    recordScenes(frame, curPipeline);
    recordViewports(frame, curPipeline, vpIndices);
}

} // namespace trc
