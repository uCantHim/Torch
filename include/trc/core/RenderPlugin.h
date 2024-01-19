#pragma once

#include "trc/Types.h"

namespace trc
{
    class Camera;
    class DescriptorRegistry;
    class RenderGraph;
    class RenderTarget;
    class ResourceConfig;
    class SceneBase;
    class TaskQueue;

    class ViewportResources
    {
    public:
        virtual void createTasks(SceneBase& scene, TaskQueue& taskQueue) = 0;
    };

    /**
     * @brief A plugin to the render pipeline
     */
    class RenderPlugin
    {
    public:
        RenderPlugin() = default;

        virtual ~RenderPlugin() noexcept = default;

        // --- Setup ---

        virtual void registerRenderStages(RenderGraph& renderGraph) = 0;
        virtual void defineResources(ResourceConfig& config) = 0;

        virtual void registerSceneModules(SceneBase& scene) = 0;

        // --- Callbacks into rendering pipeline ---

        virtual void preDrawUpdate(const Camera& camera, const SceneBase& scene) = 0;
        virtual void postDrawUpdate(const Camera& camera, const SceneBase& scene) = 0;

        // --- Drawing ---

        virtual void createTasks(SceneBase& scene, TaskQueue& taskQueue) = 0;

        virtual auto createViewportResources(const RenderTarget& target)
            -> u_ptr<ViewportResources> = 0;
    };

    class RenderPluginRegistry
    {
    public:
        void clear();
    };
} // namespace trc
