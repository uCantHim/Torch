#pragma once

#include "trc/base/FrameSpecificObject.h"

#include "trc/Types.h"
#include "trc/VulkanInclude.h"

namespace trc
{
    class DescriptorProviderInterface
    {
    public:
        virtual ~DescriptorProviderInterface() = default;

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
        explicit DescriptorProvider(vk::DescriptorSet set);

        void bindDescriptorSet(
            vk::CommandBuffer cmdBuf,
            vk::PipelineBindPoint bindPoint,
            vk::PipelineLayout pipelineLayout,
            ui32 setIndex
        ) const override;

        void setDescriptorSet(vk::DescriptorSet newSet);

    private:
        vk::DescriptorSet set;
    };

    /**
     * @brief A provider for frame-specific descriptor sets
     */
    class FrameSpecificDescriptorProvider : public DescriptorProviderInterface
    {
    public:
        explicit FrameSpecificDescriptorProvider(FrameSpecific<vk::DescriptorSet> set);

        /**
         * @brief Convert handle types to plain handles
         */
        template<typename T>
            requires requires (T a) { { *a } -> std::convertible_to<vk::DescriptorSet>; }
        FrameSpecificDescriptorProvider(FrameSpecific<T>& sets)
            :
            FrameSpecificDescriptorProvider(
                { sets.getFrameClock(), [&](ui32 i) { return *sets.getAt(i); } }
            )
        {}

        void bindDescriptorSet(
            vk::CommandBuffer cmdBuf,
            vk::PipelineBindPoint bindPoint,
            vk::PipelineLayout pipelineLayout,
            ui32 setIndex
        ) const override;

        void setDescriptorSet(FrameSpecific<vk::DescriptorSet> newSet);

    private:
        FrameSpecific<vk::DescriptorSet> set;
    };
}
