#pragma once

#include <vector>

#include "trc/base/FrameSpecificObject.h"
#include "trc/core/Pipeline.h"
#include "trc/core/RenderGraph.h"
#include "trc/core/RenderPlugin.h"
#include "trc/core/RenderTarget.h"
#include "trc/core/ResourceConfig.h"

namespace trc
{
    class Instance;

    /**
     * @brief An instantiation of a render pipeline for a specific viewport.
     */
    class ViewportConfig
    {
    public:
        ViewportConfig(Viewport viewport,
                       s_ptr<RenderGraph> renderGraph,
                       ResourceStorage resourceStorage,
                       std::vector<u_ptr<DrawConfig>> pluginConfigs);

        void update(const Device& device, SceneBase& scene, const Camera& camera);
        void createTasks(SceneBase& scene, TaskQueue& taskQueue);

        auto getViewport() const -> Viewport;
        auto getRenderGraph() const -> const RenderGraph&;
        auto getResources() -> ResourceStorage&;
        auto getResources() const -> const ResourceStorage&;

    private:
        Viewport viewport;

        s_ptr<RenderGraph> renderGraph;
        ResourceStorage resources;
        std::vector<u_ptr<DrawConfig>> pluginConfigs;
    };

    /**
     * @brief A configuration of an entire render cycle
     */
    class RenderConfig
    {
    public:
        /**
         * @brief Construct a render pipeline
         */
        explicit RenderConfig(const Instance& instance);

        /**
         * @brief Add a render plugin to the pipeline
         *
         * It is advised to register all plugins at the render pipeline first
         * (in a sort of initialization step), and only after that create scenes
         * or viewports from it. Otherwise your plugin will not be taken into
         * account during creation of those objects.
         */
        void registerPlugin(s_ptr<RenderPlugin> plugin);

        /**
         * @brief Instantiate the render pipeline for a viewport
         */
        auto makeViewport(const Device& device, Viewport viewport)
            -> u_ptr<ViewportConfig>;

        /**
         * @brief Instantiate the render pipeline for all images of a render
         *        target.
         */
        auto makeViewports(const Device& device,
                           const RenderTarget& newTarget,
                           ivec2 renderAreaOffset,
                           uvec2 renderArea)
            -> FrameSpecific<u_ptr<ViewportConfig>>;

        auto getRenderGraph() -> RenderGraph&;
        auto getRenderGraph() const -> const RenderGraph&;

        auto getResourceConfig() -> ResourceConfig&;
        auto getResourceConfig() const -> const ResourceConfig&;

    protected:
        s_ptr<RenderGraph> renderGraph;
        s_ptr<ResourceConfig> resourceConfig;
        s_ptr<PipelineStorage> pipelineStorage;

        std::vector<s_ptr<RenderPlugin>> plugins;
    };
} // namespace trc
