#pragma once

#include <trc/base/Buffer.h>
#include <trc/base/Device.h>

#include "trc/core/RenderPipelineTasks.h"
#include "trc/core/RenderPluginContext.h"
#include "trc/core/RenderTarget.h"

namespace trc
{
    class Instance;
    class RenderGraph;
    class ResourceConfig;
    class ResourceStorage;
    class SceneBase;

    namespace impl
    {
        /** So clang-tidy will shut up. */
        struct IsVirtual
        {
            virtual ~IsVirtual() noexcept = default;

            IsVirtual() = default;

            IsVirtual(const IsVirtual&) = delete;
            IsVirtual(IsVirtual&&) noexcept = delete;
            IsVirtual& operator=(const IsVirtual&) = delete;
            IsVirtual& operator=(IsVirtual&&) noexcept = delete;
        };
    } // namespace impl

    class RenderPlugin;

    /**
     * @brief Information provided when building render plugins.
     */
    struct PluginBuildContext
    {
        auto instance() -> const Instance&;
        auto device() -> const Device&;

        /** Number of buffered frames in the render target. */
        auto numFrames() -> ui32;

        /** Is also the maximum number of scenes. */
        auto numViewports() -> ui32;

        auto renderTarget() -> RenderTarget&;
        auto assetManager() -> AssetManager&;

    private:
        struct Self;
        s_ptr<Self> self;
    };

    /**
     * @brief A function signature for render plugin factories.
     */
    using PluginBuilder = std::function<u_ptr<RenderPlugin>(PluginBuildContext&)>;

    class GlobalResources : impl::IsVirtual
    {
    public:
        virtual void registerResources(ResourceStorage& resources) = 0;

        virtual void hostUpdate(RenderPipelineContext& ctx) = 0;
        virtual void createTasks(GlobalUpdateTaskQueue& queue) = 0;
    };

    class SceneResources : impl::IsVirtual
    {
    public:
        virtual void registerResources(ResourceStorage& resources) = 0;

        virtual void hostUpdate(SceneContext& ctx) = 0;
        virtual void createTasks(SceneUpdateTaskQueue& queue) = 0;
    };

    /**
     * Manages per-viewport data.
     */
    class ViewportResources : impl::IsVirtual
    {
    public:
        virtual void registerResources(ResourceStorage& resources) = 0;

        virtual void hostUpdate(ViewportContext& ctx) = 0;
        virtual void createTasks(ViewportDrawTaskQueue& queue, ViewportContext& ctx) = 0;

        // TODO: Something like this to enable advanced optimizations?
        //virtual bool isSwapchainImageCountAware() const = 0;
    };

    class RenderPlugin : impl::IsVirtual
    {
    public:
        virtual void defineRenderStages(RenderGraph& graph) = 0;
        virtual void defineResources(ResourceConfig& config) = 0;

        virtual auto createGlobalResources(RenderPipelineContext& /*ctx*/)
            -> u_ptr<GlobalResources>
        { return nullptr; }
        virtual auto createSceneResources(SceneContext& /*ctx*/)
            -> u_ptr<SceneResources>
        { return nullptr; }
        virtual auto createViewportResources(ViewportContext& /*ctx*/)
            -> u_ptr<ViewportResources>
        { return nullptr; }

        virtual auto recreateGlobalResources(u_ptr<GlobalResources> res, RenderPipelineContext& ctx)
            -> u_ptr<GlobalResources>
        { res.reset(); return createGlobalResources(ctx); }

        virtual auto recreateSceneResources(u_ptr<SceneResources> res,
                                            SceneContext& ctx)
            -> u_ptr<SceneResources>
        { res.reset(); return createSceneResources(ctx); }

        virtual auto recreateViewportResources(u_ptr<ViewportResources> res,
                                               ViewportContext& ctx)
            -> u_ptr<ViewportResources>
        { res.reset(); return createViewportResources(ctx); }
    };
} // namespace trc
