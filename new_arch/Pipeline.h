#pragma once

#include <vulkan/vulkan.hpp>
#include <vkb/VulkanBase.h>

#include "data_utils/SelfManagedObject.h"

namespace trc
{
    class PipelineLayout
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

        void addDescriptorSetLayouts();
        auto getDescriptorSetLayouts();

        void addPushConstantRanges();
        auto getPushConstantRanges();

    private:
        vk::UniquePipelineLayout layout;

        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
        std::vector<vk::PushConstantRange> pushConstantRanges;
    };

    /**
     * @brief Base class for all pipelines
     */
    class Pipeline
    {
    public:
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
         * @param bool bindStaticDescSets  If false, don't bind the pipeline's
         *                                 static descriptor sets.
         */
        void bind(vk::CommandBuffer cmdBuf) const;
        void bindDescriptorSets(vk::CommandBuffer cmdBuf) const;

        auto getLayout() const noexcept -> vk::PipelineLayout;

        void addStaticDescriptorSet(uint32_t descriptorIndex, vk::DescriptorSet set) noexcept;

    protected:
        Pipeline() = default;
        Pipeline(vk::PipelineLayout layout,
                 vk::UniquePipeline pipeline,
                 vk::PipelineBindPoint bindPoint)
            :
            layout(layout),
            pipeline(std::move(pipeline)),
            bindPoint(bindPoint)
        {}

    private:
        vk::PipelineLayout layout;
        vk::UniquePipeline pipeline;
        vk::PipelineBindPoint bindPoint;

        std::vector<std::pair<uint32_t, vk::DescriptorSet>> staticDescriptorSets;
    };

    /**
     * @brief A graphics pipeline
     */
    class GraphicsPipeline : public Pipeline,
                             public SelfManagedObject<GraphicsPipeline>
    {
    public:
        GraphicsPipeline() = default;
        GraphicsPipeline(vk::PipelineLayout layout, vk::GraphicsPipelineCreateInfo info)
            :
            Pipeline(
                layout,
                vkb::VulkanBase::getDevice()->createGraphicsPipelineUnique({}, info),
                vk::PipelineBindPoint::eGraphics
            )
        {}

        GraphicsPipeline(vk::PipelineLayout layout, vk::UniquePipeline pipeline)
            :
            Pipeline(layout, std::move(pipeline), vk::PipelineBindPoint::eGraphics)
        {}
    };

    /**
     * @brief A compute pipeline
     */
    class ComputePipeline : public Pipeline,
                            public SelfManagedObject<ComputePipeline>
    {
    public:
        ComputePipeline(vk::PipelineShaderStageCreateInfo shader, vk::PipelineLayout layout);
    };
} // namespace trc
