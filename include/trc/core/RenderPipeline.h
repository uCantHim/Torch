#pragma once

#include <span>
#include <vector>

#include <trc_util/data/IdPool.h>
#include <trc_util/data/FixedSizeVector.h>
#include <trc_util/data/Multiset.h>

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
     */
    class RenderPipelineViewport
    {
    public:
        using RenderTargetUpdateCallback
            = std::function<RenderArea(const RenderPipelineViewport&, const RenderTarget&)>;

        static GLM_CONSTEXPR const vec4 kDefaultClearColor{ 0, 0, 0, 1 };

        auto getRenderTarget() const -> const RenderTarget&;
        auto getRenderArea() const -> const RenderArea&;

        auto getCamera() -> Camera&;
        auto getScene() -> SceneBase&;

        void resize(const RenderArea& newArea);
        void setCamera(const s_ptr<Camera>& camera);
        void setScene(const s_ptr<SceneBase>& scene);

        /**
         * Set a callback that gets called when the render target to which the
         * viewport draws changes. The callback should calculate and return a
         * new render area on the new render target.
         *
         * Only one callback function can be set on a viewport, so this function
         * always overwrites the current callback.
         */
        void onRenderTargetUpdate(RenderTargetUpdateCallback callback);

    private:
        friend class RenderPipeline;
        RenderPipelineViewport(RenderPipeline& pipeline,
                               ui32 viewportIndex,
                               const RenderArea& renderArea,
                               const s_ptr<Camera>& camera,
                               const s_ptr<SceneBase>& scene,
                               vec4 clearColor = kDefaultClearColor);

        /**
         * Called by the RenderPipeline when the viewport's render target
         * changes.
         */
        auto notifyRenderTargetUpdate(const RenderTarget& newTarget) const -> RenderArea;

        auto accessParent(this auto&& self) -> decltype(auto)
        {
            if (auto p = self.parent.lock()) {
                return p->self;
            }

            throw std::runtime_error("[In RenderPipelineViewport::accessParent]: The viewport's"
                                     " parent render pipeline has been destroyed. Access to the"
                                     " viewport object is now invalid.");
        }

        struct Parent {
            RenderPipeline& self;
        };

        // Weak reference to the viewport's parent.
        //
        // See RenderPipeline::self for more documentation.
        w_ptr<Parent> parent;

        // This viewport's index in the parent's list of viewports.
        const ui32 vpIndex;

        RenderArea area;
        s_ptr<Camera> camera;
        s_ptr<SceneBase> scene;
        const vec4 clearColor;

        RenderTargetUpdateCallback renderTargetUpdateCallback;
    };

    using ViewportHandle = s_ptr<RenderPipelineViewport>;

    class RenderPipeline
    {
    public:
        RenderPipeline(const Instance& instance,
                       std::span<PluginBuilder> plugins,
                       const RenderTarget& renderTarget,
                       ui32 maxViewports);

        auto makeFrame() -> u_ptr<Frame>;

        /**
         * @brief Create a frame and submit draw commands for all viewports to it.
         */
        auto drawAllViewports() -> u_ptr<Frame>;

        /**
         * @brief Create a frame and draw a specific selection of viewports.
         *
         * @param viewport None shall be nullptr.
         *
         * @throw std::invalid_argument if any of the specified viewport handles
         *                              are nullptr.
         */
        auto draw(const vk::ArrayProxy<ViewportHandle>& viewports) -> u_ptr<Frame>;

        /**
         * @brief Draw all viewports to a frame.
         */
        void drawAllViewports(Frame& frame);

        /**
         * @brief Draw a selection of viewports to a frame.
         */
        void draw(const vk::ArrayProxy<ViewportHandle>& viewports, Frame& frame);

        /**
         * @throw std::out_of_range if the new number of viewports would exceed
         *                          `RenderPipeline::getMaxViewports`.
         */
        auto makeViewport(const RenderArea& renderArea,
                          const s_ptr<Camera>& camera,
                          const s_ptr<SceneBase>& scene,
                          vec4 clearColor = RenderPipelineViewport::kDefaultClearColor)
            -> ViewportHandle;

        auto getMaxViewports() const -> ui32;

        auto getResourceConfig() -> ResourceConfig&;
        auto getRenderGraph() -> RenderGraph&;
        auto getRenderTarget() const -> const RenderTarget&;

        /**
         * @brief Change the render target to which the pipeline draws its
         *        viewports.
         *
         * The new target must never have more buffered frames than the render
         * target for which the RenderPipeline was created.
         *
         * Note: This function can be very expensive, as it may have to
         * re-create all resources that depend on the render target, notably all
         * viewport resources.
         */
        void changeRenderTarget(const RenderTarget& newTarget);

    private:
        friend RenderPipelineViewport;

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
            std::unordered_map<s_ptr<SceneBase>, s_ptr<PerScene>> scenes;
            data::FixedSize<s_ptr<PerViewport>> viewports;
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
                                        const s_ptr<SceneBase>& scene)
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
        void useScene(const s_ptr<SceneBase>& newScene);

        /**
         * Inform the pipeline that a scene is not used anymore. If the total
         * use count of that scene drops to zero, its resources are freed.
         */
        void freeScene(const s_ptr<SceneBase>& scene);

        void recreateViewportForAllFrames(ui32 viewportIndex,
                                          const RenderArea& renderArea,
                                          const s_ptr<Camera>& camera,
                                          const s_ptr<SceneBase>& scene,
                                          vec4 clearColor);

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

        // RenderPipelineViewports created by the pipeline reference this struct
        // via a weak_pointer. This is used to ensure that a viewport never
        // accesses a parent pipeline that has already been destroyed.
        //
        // This creates a weak dependency cycle where both parent (the render
        // pipeline) and child (the viewport) reference each other via weak
        // pointers.
        //
        // Note that the `Parent` struct is merely a pointer to the parent.
        s_ptr<RenderPipelineViewport::Parent> selfReference{
            std::make_unique<RenderPipelineViewport::Parent>(*this)
        };

        const Device& device;
        const ui32 maxViewports;
        const ui32 maxRenderTargetFrames;

        RenderTarget renderTarget;

        s_ptr<RenderGraph> renderGraph;
        s_ptr<ResourceConfig> resourceConfig;
        s_ptr<PipelineStorage> pipelineStorage;

        /**
         * All other resource storages for viewports or scenes derive from this
         * one.
         */
        s_ptr<ResourceStorage> topLevelResourceStorage;

        std::vector<s_ptr<RenderPlugin>> renderPlugins;
        u_ptr<trc::FrameSpecific<PipelineInstance>> pipelinesPerFrame;

        data::Multiset<s_ptr<SceneBase>> uniqueScenes;
        data::IdPool<ui32> viewportIdPool;
        std::vector<WeakViewportHandle> allocatedViewports;
    };
} // namespace trc
