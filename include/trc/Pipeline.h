#pragma once

#include <vkb/VulkanBase.h>

#include "Types.h"
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
        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;

        Pipeline(vk::UniquePipelineLayout layout,
                 vk::UniquePipeline pipeline,
                 vk::PipelineBindPoint bindPoint);
        Pipeline(Pipeline&&) noexcept = default;
        ~Pipeline() = default;

        Pipeline& operator=(Pipeline&&) noexcept = default;

        auto operator*() const noexcept -> vk::Pipeline;
        auto get() const noexcept -> vk::Pipeline;

        /**
         * @brief Bind the pipeline and all static descriptor sets
         *
         * @param vk::CommandBuffer cmdBuf The command buffer to record the
         *                                 pipeline bind to.
         */
        void bind(vk::CommandBuffer cmdBuf) const;

        /**
         * @brief Bind all static descriptor sets specified for the pipeline
         *
         * "Static" descriptor sets are the descriptor sets that are
         * pipeline-specific rather than draw-call-specific.
         */
        void bindStaticDescriptorSets(vk::CommandBuffer cmdBuf) const;

        /**
         * @brief Supply default values to specified push constant ranges
         */
        void bindDefaultPushConstantValues(vk::CommandBuffer cmdBuf) const;

        auto getLayout() const noexcept -> vk::PipelineLayout;

        void addStaticDescriptorSet(ui32 descriptorIndex,
                                    const DescriptorProviderInterface& provider) noexcept;
        template<typename T>
        void addDefaultPushConstantValue(ui32 offset, T value, vk::ShaderStageFlags stages);

    private:
        vk::UniquePipelineLayout layout;
        vk::UniquePipeline pipeline;
        vk::PipelineBindPoint bindPoint;

        std::vector<std::pair<ui32, const DescriptorProviderInterface*>> staticDescriptorSets;
        using PushConstantValue = std::tuple<ui32, vk::ShaderStageFlags, std::vector<uint8_t>>;
        std::vector<PushConstantValue> defaultPushConstants;
    };



    template<typename T>
    void Pipeline::addDefaultPushConstantValue(ui32 offset, T value, vk::ShaderStageFlags stages)
    {
        std::vector<uint8_t> defaultValue;
        defaultValue.resize(sizeof(T));
        memcpy(defaultValue.data(), &value, sizeof(T));

        defaultPushConstants.emplace_back(offset, stages, std::move(defaultValue));
    }
} // namespace trc
