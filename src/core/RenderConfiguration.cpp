#include "trc/core/RenderConfiguration.h"

#include <cassert>
#include <source_location>
#include <string>

#include "trc/core/Frame.h"



namespace trc
{

#ifdef TRC_DEBUG
#define assert_valid_vp_handle() \
    if (this->impl == nullptr) { \
        constexpr auto loc = std::source_location::current(); \
        throw std::runtime_error( \
            "[" + std::string{loc.function_name()} + "]: " \
            + "Invalid operation: Viewport handle is null!"); \
    }
#else
#define assert_valid_vp_handle (static_cast<void>(0))
#endif

struct ViewportHandle::Impl
{
    auto getVp() const -> RenderPipeline::PerViewport& {
        auto& pipeline = parent->pipelinesPerFrame.getAt(0);
        return *pipeline.viewports[vpIndex];
    }

    RenderPipeline* parent;
    ui32 vpIndex;
};

ViewportHandle::ViewportHandle(RenderPipeline* parent, ui32 vpIndex)
    :
    impl{
        new Impl{ parent, vpIndex },
        [](Impl* impl) {
            impl->parent->freeViewport(impl->vpIndex);
            delete impl;
        }
    }
{
}

void ViewportHandle::reset()
{
    impl.reset();
}

//void ViewportHandle::draw(Frame& frame)
//{
//}

auto ViewportHandle::image() -> const RenderImage&
{
    assert_valid_vp_handle();
    return impl->getVp().info.renderImage();
}

auto ViewportHandle::renderArea() -> const RenderArea&
{
    assert_valid_vp_handle();
    return impl->getVp().info.renderArea();
}

auto ViewportHandle::camera() -> Camera&
{
    assert_valid_vp_handle();
    return impl->getVp().info.camera();
}

auto ViewportHandle::scene() -> SceneBase&
{
    assert_valid_vp_handle();
    return impl->getVp().info.scene();
}

void ViewportHandle::resize(const RenderArea& newArea)
{
    assert_valid_vp_handle();
}

void ViewportHandle::setCamera(Camera& camera)
{
    assert_valid_vp_handle();
}

void ViewportHandle::setScene(SceneBase& scene)
{
    assert_valid_vp_handle();
}



auto RenderPipelineBuilder::compile(const RenderPipelineCreateInfo& createInfo) -> u_ptr<RenderPipeline>
{
    return std::make_unique<RenderPipeline>(
        createInfo.instance,
        pluginBuilders,
        createInfo.renderTarget,
        createInfo.maxViewports
    );
}

void RenderPipelineBuilder::addPlugin(PluginBuilder builder)
{
    pluginBuilders.emplace_back(std::move(builder));
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

    // Render plugins
    renderPlugins([&]{
        std::vector<u_ptr<RenderPlugin>> res;
        PluginBuildContext ctx{};
        for (auto& builder : pluginBuilders) {
            res.emplace_back(builder(ctx));
        }
        return res;
    }()),

    // Pipeline instances per swapchain image
    pipelinesPerFrame(
        _renderTarget.getFrameClock(),
        [&](const ui32 /*frame*/) -> PipelineInstance
        {
            impl::RenderPipelineInfo info{ device, *this };
            auto resourceStorage = ResourceStorage::derive(topLevelResourceStorage);
            return {
                .global{
                    .info{ info },
                    .pluginImpls{ createPluginGlobalInstances(*resourceStorage, info) },
                    .taskQueue{},
                    .resources{ std::move(resourceStorage) },
                },
                .scenes{},     // No scenes or viewports yet.
                .viewports{},  // Create these on demand.
            };
        }
    )
{
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
    drawToFrame(*frame, getAllViewportIndices(pipelinesPerFrame.get()));

    return frame;
}

auto RenderPipeline::draw(std::span<ViewportHandle> viewports) -> u_ptr<Frame>
{
    auto frame = std::make_unique<Frame>(
        device,
        renderGraph->compile(),
        topLevelResourceStorage
    );

    drawToFrame(
        *frame,
        std::views::transform(viewports, [](auto&& vp) { return vp.impl->vpIndex; })
    );

    return frame;
}

auto RenderPipeline::makeViewport(
    const RenderArea& renderArea,
    Camera& camera,
    SceneBase& scene) -> ViewportHandle
{
    const ui32 viewportIndex = viewportIdPool.generate();
    if (viewportIndex >= maxViewports) {
        throw std::out_of_range("Too many viewports!");
    }

    uniqueScenes.emplace(&scene);
    const bool isNewScene = uniqueScenes.count(&scene) == 1;

    for (ui32 i = 0; auto img : renderTarget.getRenderImages())
    {
        auto& pipeline = pipelinesPerFrame.getAt(i);

        // Create resources specific to the scene
        if (isNewScene)
        {
            assert(!pipeline.scenes.contains(&scene));

            auto resourceStorage = ResourceStorage::derive(pipeline.global.resources);
            auto pluginInstances = createPluginSceneInstances(*resourceStorage, pipeline.global.info, scene);
            pipeline.scenes.try_emplace(&scene, std::make_unique<PerScene>(
                PerScene{
                    .info{ scene },
                    .pluginImpls{ std::move(pluginInstances) },
                    .taskQueue{},
                    .resources{ std::move(resourceStorage) },
                }
            ));
        }

        // Create resources specific to the new viewport
        assert(pipeline.viewports.size() == maxViewports);
        assert(viewportIndex < pipeline.viewports.size());
        assert(pipeline.viewports[viewportIndex] == nullptr);

        const impl::ViewportInfo info{ Viewport{ img, renderArea }, scene, camera };

        auto baseResources = pipeline.scenes.at(&scene)->resources;
        auto resourceStorage = ResourceStorage::derive(baseResources);
        auto pluginInstances = createPluginViewportInstances(*resourceStorage, pipeline.global.info, info);
        pipeline.viewports[i] = std::make_unique<PerViewport>(
            PerViewport{
                .info{ info },
                .pluginImpls{ std::move(pluginInstances) },
                .taskQueue{},
                .resources{ std::move(resourceStorage) },
            }
        );

        ++i;
    }

    return { this, viewportIndex };
}

void RenderPipeline::recordGlobal(Frame& frame, PipelineInstance& pipeline)
{
    auto& global = pipeline.global;
    auto& queue = global.taskQueue;
    for (auto& plugin : global.pluginImpls) {
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
        for (auto& plugin : scene->pluginImpls) {
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
        for (auto& plugin : vp->pluginImpls) {
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
    SceneBase& scene)
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

void RenderPipeline::drawToFrame(Frame& frame, std::ranges::range auto&& vpIndices)
{
    auto& curPipeline = pipelinesPerFrame.get();
    recordGlobal(frame, curPipeline);
    recordScenes(frame, curPipeline);
    recordViewports(frame, curPipeline, vpIndices);
}

} // namespace trc
