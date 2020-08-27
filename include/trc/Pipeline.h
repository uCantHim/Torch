#pragma once

#include <vkb/VulkanBase.h>

#include "Boilerplate.h"
#include "data_utils/SelfManagedObject.h"
#include "DescriptorProvider.h"

namespace trc
{
    class PipelineLayout : public data::SelfManagedObject<PipelineLayout>
    {
    public:
        PipelineLayout(
            std::vector<vk::DescriptorSetLayout> descriptorSetLayouts,
            std::vector<vk::PushConstantRange> pushConstantRanges)
            :
            layout(vkb::VulkanBase::getDevice()->createPipelineLayoutUnique(
                vk::PipelineLayoutCreateInfo(
                    {},
                    descriptorSetLayouts.size(), descriptorSetLayouts.data(),
                    pushConstantRanges.size(), pushConstantRanges.data()
                )
            )),
            descriptorSetLayouts(std::move(descriptorSetLayouts)),
            pushConstantRanges(std::move(pushConstantRanges))
        {
        }

        auto operator*() const noexcept -> vk::PipelineLayout;
        auto get() const noexcept -> vk::PipelineLayout;

        auto getDescriptorSetLayouts();
        auto getPushConstantRanges();

    private:
        vk::UniquePipelineLayout layout;

        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
        std::vector<vk::PushConstantRange> pushConstantRanges;
    };

    /**
     * @brief Base class for all pipelines
     */
    class Pipeline : public data::SelfManagedObject<Pipeline>
    {
    public:
        Pipeline(vk::PipelineLayout layout,
                 vk::UniquePipeline pipeline,
                 vk::PipelineBindPoint bindPoint);

        Pipeline(Pipeline&&) noexcept = default;
        Pipeline& operator=(Pipeline&&) noexcept = default;

        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;

        auto operator*() const noexcept -> vk::Pipeline;
        auto get() const noexcept -> vk::Pipeline;

        /**
         * @brief Bind the pipeline and all static descriptor sets
         *
         * @param vk::CommandBuffer cmdBuf The command buffer to record the
         *                                 pipeline bind to.
         */
        void bind(vk::CommandBuffer cmdBuf) const;
        void bindStaticDescriptorSets(vk::CommandBuffer cmdBuf) const;

        auto getLayout() const noexcept -> vk::PipelineLayout;

        void addStaticDescriptorSet(ui32 descriptorIndex,
                                    const DescriptorProviderInterface& provider) noexcept;

    private:
        vk::PipelineLayout layout;
        vk::UniquePipeline pipeline;
        vk::PipelineBindPoint bindPoint;

        std::vector<std::pair<ui32, const DescriptorProviderInterface*>> staticDescriptorSets;
    };

    extern auto makeGraphicsPipeline(
        ui32 index,
        vk::PipelineLayout layout,
        vk::UniquePipeline pipeline
    ) -> Pipeline&;

    extern auto makeGraphicsPipeline(
        ui32 index,
        vk::PipelineLayout layout,
        const vk::GraphicsPipelineCreateInfo& info
    ) -> Pipeline&;

    extern auto makeComputePipeline(
        ui32 index,
        vk::PipelineLayout layout,
        vk::UniquePipeline pipeline
    ) -> Pipeline&;

    extern auto makeComputePipeline(
        ui32 index,
        vk::PipelineLayout layout,
        const vk::ComputePipelineCreateInfo& info
    ) -> Pipeline&;
} // namespace trc
