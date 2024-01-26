#pragma once

#include "trc/Types.h"
#include "trc/VulkanInclude.h"

namespace trc
{
    class Camera;
    class Device;
    class RenderGraph;
    class ResourceConfig;
    class ResourceStorage;
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

        // --- Setup ---

        virtual void registerResources(ResourceStorage& resources) = 0;

        // --- Drawing ---

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

        virtual auto createDrawConfig(const Device& device, Viewport renderTarget)
            -> u_ptr<DrawConfig> = 0;
    };
} // namespace trc
