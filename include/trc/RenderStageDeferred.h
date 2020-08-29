#pragma once

#include <vkb/FrameSpecificObject.h>
#include <vkb/Buffer.h>
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

        auto getAttachmentImageViews(ui32 imageIndex) const noexcept
           -> const std::vector<vk::UniqueImageView>&;
        auto getInputAttachmentDescriptor() const noexcept -> const DescriptorProviderInterface&;

    private:
        // Attachments
        std::vector<std::vector<vkb::Image>> attachmentImages;
        std::vector<std::vector<vk::UniqueImageView>> attachmentImageViews;

        vk::Extent2D framebufferSize;
        vkb::FrameSpecificObject<vk::UniqueFramebuffer> framebuffers;

        std::array<vk::ClearValue, 6> clearValues;
    };

    class DeferredRenderPassDescriptor
    {
    public:
        static void init(const RenderPassDeferred& deferredPass);

        static auto getProvider() -> const DescriptorProviderInterface&;

    private:
        static inline vk::UniqueDescriptorPool descPool;
        static inline vk::UniqueDescriptorSetLayout descLayout;
        static inline std::unique_ptr<vkb::FrameSpecificObject<vk::UniqueDescriptorSet>> descSets;
        static inline std::unique_ptr<FrameSpecificDescriptorProvider> provider;

        static inline vkb::Image headPointerImage;
        static inline vkb::Image fragmentBufferImage;
        static inline vkb::Buffer fragmentAllocationBuffer;
    };
} // namespace trc
