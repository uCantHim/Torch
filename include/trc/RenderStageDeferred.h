#pragma once

#include <vkb/FrameSpecificObject.h>
#include <vkb/Image.h>

#include "RenderStage.h"
#include "RenderPass.h"

namespace trc
{
    class DeferredStage : public RenderStage
    {
    public:
        DeferredStage();
    };


    class RenderPassDeferred : public RenderPass
    {
    public:
        RenderPassDeferred();

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents) override;
        void end(vk::CommandBuffer cmdBuf) override;

        auto getInputAttachmentDescriptor() const noexcept -> const DescriptorProviderInterface&;

    private:
        class InputAttachmentDescriptorProvider : public DescriptorProviderInterface
        {
        public:
            InputAttachmentDescriptorProvider(RenderPassDeferred& renderPass)
                : renderPass(&renderPass) {}

            auto getDescriptorSet() const noexcept -> vk::DescriptorSet override;
            auto getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout override;
            void bindDescriptorSet(
                vk::CommandBuffer cmdBuf,
                vk::PipelineBindPoint bindPoint,
                vk::PipelineLayout pipelineLayout,
                ui32 setIndex
            ) const override;

        private:
            RenderPassDeferred* renderPass;
        };

        // Attachments
        std::vector<std::vector<vkb::Image>> attachmentImages;
        std::vector<std::vector<vk::UniqueImageView>> attachmentImageViews;

        vk::Extent2D framebufferSize;
        vkb::FrameSpecificObject<vk::UniqueFramebuffer> framebuffers;

        std::array<vk::ClearValue, 6> clearValues;

        // Descriptors
        void createInputAttachmentDescriptors();
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout inputAttachmentLayout;
        vkb::FrameSpecificObject<vk::UniqueDescriptorSet> inputAttachmentSets;
        InputAttachmentDescriptorProvider descriptorProvider;
    };
} // namespace trc
