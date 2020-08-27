#pragma once

#include <vkb/VulkanBase.h>

#include "Boilerplate.h"
#include "data_utils/SelfManagedObject.h"
#include "DescriptorProvider.h"

namespace trc
{
    /**
     * @brief Helper to create a pipeline layout
     *
     * @param descriptorSetLayouts List of descriptor set layouts in the
     *                             pipeline
     * @param pushConstantRanges   List of push constant ranges in the
     *                             pipeline
     *
     * @return vk::UniquePipeline
     */
    inline auto makePipelineLayout(
        const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts,
        const std::vector<vk::PushConstantRange>& pushConstantRanges)
    {
        return vkb::VulkanBase::getDevice()->createPipelineLayoutUnique(
            vk::PipelineLayoutCreateInfo(
                {},
                descriptorSetLayouts.size(), descriptorSetLayouts.data(),
                pushConstantRanges.size(), pushConstantRanges.data()
            )
        );
    }

    /**
     * @brief Base class for all pipelines
     */
    class Pipeline : public data::SelfManagedObject<Pipeline>
    {
    public:
        Pipeline(vk::UniquePipelineLayout layout,
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
        vk::UniquePipelineLayout layout;
        vk::UniquePipeline pipeline;
        vk::PipelineBindPoint bindPoint;

        std::vector<std::pair<ui32, const DescriptorProviderInterface*>> staticDescriptorSets;
    };

    extern auto makeGraphicsPipeline(
        ui32 index,
        vk::UniquePipelineLayout layout,
        vk::UniquePipeline pipeline
    ) -> Pipeline&;

    extern auto makeGraphicsPipeline(
        ui32 index,
        vk::UniquePipelineLayout layout,
        const vk::GraphicsPipelineCreateInfo& info
    ) -> Pipeline&;

    extern auto makeComputePipeline(
        ui32 index,
        vk::UniquePipelineLayout layout,
        vk::UniquePipeline pipeline
    ) -> Pipeline&;

    extern auto makeComputePipeline(
        ui32 index,
        vk::UniquePipelineLayout layout,
        const vk::ComputePipelineCreateInfo& info
    ) -> Pipeline&;
} // namespace trc
