#pragma once

#include "TorchResources.h"

namespace trc
{
    class MorpholocialAntiAliasingStage : public RenderStage
    {
    public:
        MorpholocialAntiAliasingStage()
            : RenderStage(RenderStageType::at(RenderStageTypes::ePostProcessing))
        {}
    };

    /**
     * MLAA
     */
    class MorpholocialAntiAliasingDescriptor : public DescriptorProviderInterface
    {
    public:
        MorpholocialAntiAliasingDescriptor(
            vk::Extent2D framebufferSize,
            std::vector<std::pair<vk::Sampler, vk::ImageView>> depthImages);

        auto getDescriptorSet() const noexcept -> vk::DescriptorSet override;
        auto getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout override;

        void bindDescriptorSet(
            vk::CommandBuffer cmdBuf,
            vk::PipelineBindPoint bindPoint,
            vk::PipelineLayout pipelineLayout,
            ui32 setIndex
        ) const override;

    private:
        void createResources(vk::Extent2D framebufferSize);
        void createDescriptors(std::vector<std::pair<vk::Sampler, vk::ImageView>> depthImages);


        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
        vkb::FrameSpecificObject<vk::UniqueDescriptorSet> descSets;

        vkb::Image precomputedAreaTexture;
        vk::UniqueImageView precomputedAreaTextureView;
        vkb::FrameSpecificObject<vkb::Image> edgeImages;
        vkb::FrameSpecificObject<vk::UniqueImageView> edgeImageViews;
        vkb::FrameSpecificObject<vkb::Image> areaImages;
        vkb::FrameSpecificObject<vk::UniqueImageView> areaImageViews;
    };
}
