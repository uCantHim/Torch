#pragma once

#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <trc_util/data/IdPool.h>

#include "trc/Types.h"
#include "trc/core/RenderPipelineTasks.h"
#include "trc/core/RenderPlugin.h"
#include "trc/core/ResourceConfig.h"

namespace trc
{
    class AssetManager;
    class Frame;
    class Instance;
    class RenderTarget;
    class SceneBase;

    class RenderPipelineBuilder;
    class RenderPipeline;
    class ViewportHandle;

    struct RenderPipelineCreateInfo
    {
        const Instance& instance;
        const RenderTarget& renderTarget;
        ui32 maxViewports{ 1 };
    };

    class RenderPipelineBuilder
    {
    public:
        RenderPipelineBuilder() = default;

        auto compile(const RenderPipelineCreateInfo& createInfo) -> u_ptr<RenderPipeline>;

        void addPlugin(PluginBuilder builder);

    private:
        std::vector<PluginBuilder> pluginBuilders;
    };

    /**
     * @brief Exposed by a RenderPipeline to allow viewport manipulation.
     *
     * May be null.
     */
    class ViewportHandle
    {
    public:
        /**
         * @brief Construct a null handle.
         */
        ViewportHandle() = default;

        operator bool() const {
            return impl != nullptr;
        }

        bool operator==(const ViewportHandle&) const = default;

        void reset();

        auto draw() -> u_ptr<Frame>;

        auto image() -> const RenderImage&;
        auto renderArea() -> const RenderArea&;

        auto camera() -> Camera&;
        auto scene() -> SceneBase&;

        void resize(const RenderArea& newArea);
        void setRenderTarget(const RenderTarget& newTarget);
        void setCamera(Camera& camera);
        void setScene(SceneBase& scene);

    private:
        friend class RenderPipeline;
        ViewportHandle(RenderPipeline* parent, ui32 vpIndex);

        // Treat the ViewportHandle as a reference with shared_ptr semantics.
        struct Impl;
        s_ptr<Impl> impl;
    };

    class RenderPipeline
    {
    public:
        RenderPipeline(const Instance& instance,
                       std::span<PluginBuilder> plugins,
                       const RenderTarget& renderTarget,
                       ui32 maxViewports);

        /**
         * @brief Create a frame and submit draw commands for all viewports to it.
         */
        auto draw() -> u_ptr<Frame>;

        /**
         * @brief Create a frame and draw a specific selection of viewports.
         */
        auto draw(std::span<ViewportHandle> viewports) -> u_ptr<Frame>;

        /**
         * @throw std::out_of_range
         */
        auto makeViewport(const RenderArea& renderArea,
                          Camera& camera,
                          SceneBase& scene)
            -> ViewportHandle;

        auto getResourceConfig() -> ResourceConfig&;
        auto getRenderGraph() -> RenderGraph&;

    private:
        friend ViewportHandle;        // Accesses RenderPipeline::pipelinesPerFrame
        friend ViewportHandle::Impl;  // Accesses RenderPipeline::freeViewport

        struct Global
        {
            impl::RenderPipelineInfo info;

            std::vector<u_ptr<GlobalResources>> pluginImpls;
            GlobalUpdateTaskQueue taskQueue;
            s_ptr<ResourceStorage> resources;
        };

        struct PerScene
        {
            impl::SceneInfo info;

            std::vector<u_ptr<SceneResources>> pluginImpls;
            SceneUpdateTaskQueue taskQueue;
            s_ptr<ResourceStorage> resources;
        };

        struct PerViewport
        {
            impl::ViewportInfo info;

            std::vector<u_ptr<ViewportResources>> pluginImpls;
            ViewportDrawTaskQueue taskQueue;
            s_ptr<ResourceStorage> resources;
        };

        struct PipelineInstance
        {
            Global global;
            std::unordered_map<SceneBase*, u_ptr<PerScene>> scenes;
            std::vector<u_ptr<PerViewport>> viewports;
        };

        static void recordGlobal(Frame& frame, PipelineInstance& pipeline);
        static void recordScenes(Frame& frame, PipelineInstance& pipeline);
        static void recordViewports(Frame& frame,
                                    PipelineInstance& pipeline,
                                    std::ranges::range auto&& vpIndices);

        auto createPluginGlobalInstances(ResourceStorage& resourceStorage,
                                         const impl::RenderPipelineInfo& pipeline)
            -> std::vector<u_ptr<GlobalResources>>;
        auto createPluginSceneInstances(ResourceStorage& resourceStorage,
                                        const impl::RenderPipelineInfo& pipeline,
                                        SceneBase& scene)
            -> std::vector<u_ptr<SceneResources>>;
        auto createPluginViewportInstances(ResourceStorage& resourceStorage,
                                           const impl::RenderPipelineInfo& pipeline,
                                           const impl::ViewportInfo& viewport)
            -> std::vector<u_ptr<ViewportResources>>;

        void freeViewport(ui32 viewportIndex);

        void drawToFrame(Frame& frame, std::ranges::range auto&& vpIndices);

        const Device& device;
        const ui32 maxViewports;

        RenderTarget renderTarget;

        s_ptr<RenderGraph> renderGraph;
        s_ptr<ResourceConfig> resourceConfig;
        s_ptr<PipelineStorage> pipelineStorage;

        /**
         * All other resource storages for viewports or scenes derive from this
         * one.
         */
        s_ptr<ResourceStorage> topLevelResourceStorage;

        std::vector<u_ptr<RenderPlugin>> renderPlugins;
        trc::FrameSpecific<PipelineInstance> pipelinesPerFrame;

        std::unordered_multiset<SceneBase*> uniqueScenes;
        data::IdPool<ui32> viewportIdPool;
    };
} // namespace trc
