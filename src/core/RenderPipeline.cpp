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
    const s_ptr<SceneBase>& scene,
    vec4 clearColor)
    :
    parent(pipeline),
    vpIndex(viewportIndex),
    area(renderArea),
    camera(camera),
    scene(scene),
    clearColor(clearColor),
    renderTargetUpdateCallback([](auto& vp, auto&){ return vp.area; })
{
    assert_arg(camera != nullptr);
    assert_arg(scene != nullptr);
}

auto RenderPipelineViewport::getRenderTarget() const -> const RenderTarget&
{
    return parent.getRenderTarget();
}

auto RenderPipelineViewport::getRenderArea() const -> const RenderArea&
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
    parent.recreateViewportForAllFrames(vpIndex, newArea, camera, scene, clearColor);
    area = newArea;
}

void RenderPipelineViewport::setCamera(const s_ptr<Camera>& newCamera)
{
    assert(newCamera != nullptr);
    parent.recreateViewportForAllFrames(vpIndex, area, newCamera, scene, clearColor);
    camera = newCamera;
}

void RenderPipelineViewport::setScene(const s_ptr<SceneBase>& newScene)
{
    assert(newScene != nullptr);
    parent.recreateViewportForAllFrames(vpIndex, area, camera, newScene, clearColor);
    scene = newScene;
}

void RenderPipelineViewport::onRenderTargetUpdate(RenderTargetUpdateCallback callback)
{
    renderTargetUpdateCallback = std::move(callback);
}

auto RenderPipelineViewport::notifyRenderTargetUpdate(const RenderTarget& newTarget) const
    -> RenderArea
{
    return renderTargetUpdateCallback(*this, newTarget);
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
    maxRenderTargetFrames(_renderTarget.getFrameClock().getFrameCount()),

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

auto RenderPipeline::makeFrame() -> u_ptr<Frame>
{
    return std::make_unique<Frame>(device, topLevelResourceStorage, *renderGraph);
}

auto RenderPipeline::drawAllViewports() -> u_ptr<Frame>
{
    auto frame = makeFrame();
    drawAllViewports(*frame);

    return frame;
}

auto RenderPipeline::draw(const vk::ArrayProxy<ViewportHandle>& viewports) -> u_ptr<Frame>
{
    assert_arg(std::ranges::all_of(viewports, [](auto& v){ return v != nullptr; }));

    auto frame = makeFrame();
    draw(viewports, *frame);

    return frame;
}

void RenderPipeline::drawAllViewports(Frame& frame)
{
    draw(std::ranges::to<std::vector>(getUsedViewports()), frame);
}

void RenderPipeline::draw(const vk::ArrayProxy<ViewportHandle>& viewports, Frame& frame)
{
    assert_arg(std::ranges::all_of(viewports, [](auto& v){ return v != nullptr; }));

    frame.mergeRenderGraph(*renderGraph);
    drawToFrame(
        frame,
        std::views::transform(viewports, [](auto&& vp) { return vp->vpIndex; })
    );
}

auto RenderPipeline::makeViewport(
    const RenderArea& renderArea,
    const s_ptr<Camera>& camera,
    const s_ptr<SceneBase>& scene,
    vec4 clearColor) -> ViewportHandle
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

    // Create resources specific to the new viewport.
    recreateViewportForAllFrames(viewportIndex, renderArea, camera, scene, clearColor);

    // Create a handle to the viewport.
    auto vp = std::shared_ptr<RenderPipelineViewport>{
        new RenderPipelineViewport{ *this, viewportIndex, renderArea, camera, scene },
        [this, viewportIndex](RenderPipelineViewport* hnd)
        {
            this->freeScene(hnd->scene);
            this->freeViewport(viewportIndex);
            delete hnd;
        }
    };

    // Store weak reference to the viewport so we can look it up later when
    // viewports have to be re-created.
    allocatedViewports.emplace_back(vp);

    // Maintenance: Remove dangling weak references from the list.
    allocatedViewports = std::ranges::to<std::vector>(
        std::views::filter(allocatedViewports, [](auto&& v){ return !!v.lock(); })
    );

    return vp;
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
    // We never re-created the render plugins. That means that the original
    // number of swapchain frames must never be exceeded because plugins can use
    // that number to determine sizes of resource pools.
    assert_arg(newTarget.getFrameClock().getFrameCount() <= maxRenderTargetFrames);

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
        assert(vp != nullptr);

        const auto newArea = vp->notifyRenderTargetUpdate(newTarget);
        useScene(vp->scene);
        recreateViewportForAllFrames(
            vp->vpIndex,
            newArea,
            vp->camera,
            vp->scene,
            vp->clearColor
        );
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
        [&](ui32 i) -> PipelineInstance
        {
            auto resourceStorage = ResourceStorage::derive(topLevelResourceStorage);
            impl::RenderPipelineInfo info{ device, renderTarget.getRenderImage(i), *this };
            return {
                .global{
                    .info{ info },
                    .pluginImpls{},  // Are created after the plugins have been
                                     // created. See constructor body below.
                    .taskQueue{},
                    .resources{ std::move(resourceStorage) },
                },
                .scenes{},                   // No scenes or viewports yet. Create
                .viewports{ maxViewports },  // these on demand.
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

void RenderPipeline::recreateViewportForAllFrames(
    ui32 viewportIndex,
    const RenderArea& renderArea,
    const s_ptr<Camera>& camera,
    const s_ptr<SceneBase>& scene,
    vec4 clearColor)
{
    assert(camera != nullptr);
    assert(scene != nullptr);

    for (auto [img, pipeline] : std::views::zip(renderTarget.getRenderImages(),
                                                *pipelinesPerFrame))
    {
        assert(pipeline.viewports.size() == maxViewports);
        assert(pipeline.viewports.size() > viewportIndex);

        pipeline.viewports[viewportIndex].reset();

        const impl::ViewportInfo vpInfo{ Viewport{ img, renderArea }, clearColor, camera, scene };

        auto baseResources = pipeline.scenes.at(scene)->resources;
        auto resourceStorage = ResourceStorage::derive(baseResources);
        auto pluginInstances = createPluginViewportInstances(
            *resourceStorage,
            pipeline.global.info,
            vpInfo
        );
        pipeline.viewports[viewportIndex] = std::make_unique<PerViewport>(
            PerViewport{
                .info{ vpInfo },
                .pluginImpls{ std::move(pluginInstances) },
                .taskQueue{},
                .resources{ std::move(resourceStorage) },
            }
        );
    }
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
