#pragma once

#include <vkb/VulkanBase.h>
#include <vkb/Image.h>
#include <vkb/Buffer.h>
#include <vkb/FrameSpecificObject.h>

#include "Scene.h"
#include "CommandCollector.h"
#include "PipelineDefinitions.h"

namespace trc
{
    class Renderer : public vkb::SwapchainDependentResource
    {
    public:
        Renderer();

        void drawFrame(Scene& scene, const Camera& camera);

    private:
        void signalRecreateRequired() override;
        void recreate(vkb::Swapchain& swapchain) override;
        void signalRecreateFinished() override;

        // Synchronization
        void createSemaphores();
        vkb::FrameSpecificObject<vk::UniqueSemaphore> imageAcquireSemaphores;
        vkb::FrameSpecificObject<vk::UniqueSemaphore> renderFinishedSemaphores;
        vkb::FrameSpecificObject<vk::UniqueFence> frameInFlightFences;

        // Render passes
        std::unique_ptr<RenderPassDeferred> deferredPass;

        // General descriptor set
        void createDescriptors();
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorSet descSet;
        DescriptorProvider cameraDescriptorProvider{ {}, {} };

        void bindLightBuffer(vk::Buffer lightBuffer);
        vk::Buffer cachedLightBuffer;

        void updateCameraMatrixBuffer(const Camera& camera);
        vkb::Buffer cameraMatrixBuffer;

        // Other things
        CommandCollector collector;

        vkb::DeviceLocalBuffer fullscreenQuadVertexBuffer;
    };
}
