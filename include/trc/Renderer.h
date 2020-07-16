#pragma once

#include <vkb/VulkanBase.h>
#include <vkb/Image.h>
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

        void drawFrame(Scene& scene);

    private:
        void signalRecreateRequired() override;
        void recreate(vkb::Swapchain& swapchain) override;
        void signalRecreateFinished() override;

        void createSemaphores();
        void createFramebuffer();
        std::vector<vkb::FrameSpecificObject<vkb::Image>> framebufferAttachmentImages;
        std::vector<vkb::FrameSpecificObject<vk::UniqueImageView>> framebufferImageViews;

        CommandCollector collector;
        vkb::FrameSpecificObject<vk::UniqueFramebuffer> framebuffers;
        vkb::FrameSpecificObject<vk::UniqueSemaphore> imageAcquireSemaphores;
        vkb::FrameSpecificObject<vk::UniqueSemaphore> renderFinishedSemaphores;
        vkb::FrameSpecificObject<vk::UniqueFence> frameInFlightFences;
    };
}
