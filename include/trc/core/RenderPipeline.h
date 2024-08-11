#pragma once

#include <span>
#include <vector>

#include <trc_util/data/IdPool.h>

#include "trc/Types.h"
#include "trc/core/RenderPipelineTasks.h"
#include "trc/core/RenderPlugin.h"
#include "trc/core/ResourceConfig.h"
#include "trc/util/Multiset.h"

namespace trc
{
    class AssetManager;
    class Frame;
    class Instance;
    class RenderTarget;
    class SceneBase;

    class RenderPipelineBuilder;
    class RenderPipeline;

    struct RenderPipelineCreateInfo
    {
        const Instance& instance;
        const RenderTarget& renderTarget;
        ui32 maxViewports{ 1 };
    };

    class RenderPipelineBuilder
    {
    public:
        using Self = RenderPipelineBuilder;

        RenderPipelineBuilder() = default;

        auto build(const RenderPipelineCreateInfo& createInfo) -> u_ptr<RenderPipeline>;

        auto addPlugin(PluginBuilder builder) -> Self&;

    private:
        std::vector<PluginBuilder> pluginBuilders;
    };

    inline auto buildRenderPipeline() -> RenderPipelineBuilder {
        return RenderPipelineBuilder{};
    }

    /**
     * @brief Exposed by a RenderPipeline to allow viewport manipulation.
     *
     * May be null.
     */
    class RenderPipelineViewport
    {
    public:
        auto getRenderTarget() -> const RenderTarget&;
        auto getRenderArea() -> const RenderArea&;

        auto getCamera() -> Camera&;
        auto getScene() -> SceneBase&;

        void resize(const RenderArea& newArea);
        void setCamera(Camera& camera);
        void setScene(SceneBase& scene);

    private:
        friend class RenderPipeline;
        RenderPipelineViewport(RenderPipeline& pipeline,
                               ui32 viewportIndex,
                               const RenderArea& renderArea,
                               Camera& camera,
                               SceneBase& scene);

        RenderPipeline& parent;
        const ui32 vpIndex;

        RenderArea area;
        Camera& _camera;
        SceneBase& _scene;
    };

    using ViewportHandle = s_ptr<RenderPipelineViewport>;

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
        auto draw(const vk::ArrayProxy<ViewportHandle>& viewports) -> u_ptr<Frame>;

        /**
         * @throw std::out_of_range if the new number of viewports would exceed
         *                          `RenderPipeline::getMaxViewports`.
         */
        auto makeViewport(const RenderArea& renderArea,
                          Camera& camera,
                          SceneBase& scene)
            -> ViewportHandle;

        auto getMaxViewports() const -> ui32;

        auto getResourceConfig() -> ResourceConfig&;
        auto getRenderGraph() -> RenderGraph&;
        auto getRenderTarget() const -> const RenderTarget&;

        /**
         * Note: This function may be very expensive, as it has to re-create all
         * resources that depend on the render target.
         */
        void changeRenderTarget(const RenderTarget& newTarget);

    private:
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
            std::vector<u_ptr<PerViewport>> viewports;  // Fixed size
        };

        static void recordGlobal(Frame& frame, PipelineInstance& pipeline);
        static void recordScenes(Frame& frame, PipelineInstance& pipeline);
        static void recordViewports(Frame& frame,
                                    PipelineInstance& pipeline,
                                    std::ranges::range auto&& vpIndices);

        void initForRenderTarget(const RenderTarget& target);

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

        /**
         * Inform the pipeline that a scene is used in a viewport and needs
         * backing resources.
         *
         * Does nothing if the scene is already known to the pipeline.
         */
        void useScene(SceneBase& newScene);

        /**
         * Inform the pipeline that a scene is not used anymore. If the total
         * use count of that scene drops to zero, its resources are freed.
         */
        void freeScene(SceneBase& scene);

        auto instantiateViewport(PipelineInstance& parent,
                                 const RenderImage& img,
                                 const RenderArea& renderArea,
                                 Camera& camera,
                                 SceneBase& scene)
            -> u_ptr<PerViewport>;

        /**
         * @return All viewport indices that are currently in use.
         */
        auto getUsedViewports() const -> std::generator<ViewportHandle>;

        /**
         * Destroy a viewport's resources.
         */
        void freeViewport(ui32 viewportIndex);

        /**
         * Create tasks for a selection of viewports and submit them to a frame.
         */
        void drawToFrame(Frame& frame, std::ranges::range auto&& vpIndices);

        using WeakViewportHandle = w_ptr<RenderPipelineViewport>;

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
        u_ptr<trc::FrameSpecific<PipelineInstance>> pipelinesPerFrame;

        util::Multiset<SceneBase*> uniqueScenes;
        data::IdPool<ui32> viewportIdPool;
        std::vector<WeakViewportHandle> allocatedViewports;  // Fixed size
    };
} // namespace trc
