#pragma once

#include <vkb/VulkanBase.h>
#include <vkb/Image.h>
#include <vkb/Buffer.h>
#include <vkb/FrameSpecificObject.h>

#include "Scene.h"
#include "CommandCollector.h"
#include "RenderPassDefinitions.h"

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

        // Framebuffer
        void createFramebuffer();
        std::vector<vkb::FrameSpecificObject<vkb::Image>> framebufferAttachmentImages;
        std::vector<vkb::FrameSpecificObject<vk::UniqueImageView>> framebufferImageViews;
        vkb::FrameSpecificObject<vk::UniqueFramebuffer> framebuffers;

        // General descriptor set
        void createDescriptors();
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorSet descSet;

        void updateCameraMatrixBuffer(const Camera& camera);
        vkb::Buffer cameraMatrixBuffer;

        // Other things
        CommandCollector collector;
    };
}
