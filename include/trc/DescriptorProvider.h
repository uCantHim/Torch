#pragma once

#include <vkb/FrameSpecificObject.h>

#include "Boilerplate.h"

namespace trc
{
    struct Descriptor
    {
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
    };

    class DescriptorProviderInterface
    {
    public:
        virtual auto getDescriptorSet() const noexcept -> vk::DescriptorSet = 0;
        virtual auto getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout = 0;

        virtual void bindDescriptorSet(
            vk::CommandBuffer cmdBuf,
            vk::PipelineBindPoint bindPoint,
            vk::PipelineLayout pipelineLayout,
            ui32 setIndex
        ) const = 0;
    };

    class DescriptorProvider : public DescriptorProviderInterface
    {
    public:
        DescriptorProvider(vk::DescriptorSetLayout layout, vk::DescriptorSet set);

        auto getDescriptorSet() const noexcept -> vk::DescriptorSet override;
        auto getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout override;
        void bindDescriptorSet(
            vk::CommandBuffer cmdBuf,
            vk::PipelineBindPoint bindPoint,
            vk::PipelineLayout pipelineLayout,
            ui32 setIndex
        ) const override;

        void setDescriptorSet(vk::DescriptorSet newSet);
        void setDescriptorSetLayout(vk::DescriptorSetLayout newLayout);

    private:
        vk::DescriptorSetLayout layout;
        vk::DescriptorSet set;
    };

    /**
     * @brief A provider for frame-specific descriptor sets
     */
    class FrameSpecificDescriptorProvider : public DescriptorProviderInterface
    {
    public:
        FrameSpecificDescriptorProvider(
            vk::DescriptorSetLayout layout,
            vkb::FrameSpecificObject<vk::DescriptorSet> set);

        auto getDescriptorSet() const noexcept -> vk::DescriptorSet override;
        auto getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout override;
        void bindDescriptorSet(
            vk::CommandBuffer cmdBuf,
            vk::PipelineBindPoint bindPoint,
            vk::PipelineLayout pipelineLayout,
            ui32 setIndex
        ) const override;

        void setDescriptorSet(vkb::FrameSpecificObject<vk::DescriptorSet> newSet);
        void setDescriptorSetLayout(vk::DescriptorSetLayout newLayout);

    private:
        vk::DescriptorSetLayout layout;
        vkb::FrameSpecificObject<vk::DescriptorSet> set;
    };
}
