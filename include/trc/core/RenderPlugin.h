#pragma once

#include "trc/Types.h"
#include "trc/VulkanInclude.h"
#include "trc/core/ResourceConfig.h"

namespace trc
{
    class Camera;
    class Device;
    class DescriptorRegistry;
    class RenderGraph;
    class RenderTarget;
    class ResourceConfig;
    class SceneBase;
    class TaskQueue;

    /**
     * A render target, but with a different name
     */
    struct Viewport
    {
        vk::Image image;
        vk::ImageView imageView;
        ivec2 offset;
        uvec2 size;
    };

    /**
     * Per-plugin implementation of a full rendering pipeline for a single
     * target image
     */
    class DrawConfig
    {
    public:
        virtual ~DrawConfig() noexcept = default;

        virtual void registerResources(ResourceStorage& resources) = 0;

        virtual void update(SceneBase& scene, const Camera& camera) = 0;
        virtual void createTasks(SceneBase& scene, TaskQueue& taskQueue) = 0;
    };

    /**
     * @brief A plugin to the render pipeline
     */
    class RenderPlugin
    {
    public:
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

        virtual auto createViewportResources(const Device& device, Viewport renderTarget)
            -> u_ptr<DrawConfig> = 0;
    };
} // namespace trc
