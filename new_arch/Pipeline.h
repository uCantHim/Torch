#pragma once

#include <vulkan/vulkan.hpp>
#include <vkb/VulkanBase.h>

#include "data_utils/SelfManagedObject.h"

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

    auto operator*() const noexcept -> vk::PipelineLayout {
        return *layout;
    }

    auto get();

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

    auto operator*();
    auto get();

    /**
     * @brief Bind the pipeline and all static descriptor sets
     *
     * @param vk::CommandBuffer cmdBuf The command buffer to record the
     *                                 pipeline bind to.
     * @param bool bindStaticDescSets  If false, don't bind the pipeline's
     *                                 static descriptor sets.
     */
    void bind(vk::CommandBuffer cmdBuf, bool bindStaticDescSets = true) const noexcept
    {
        cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);

        if (bindStaticDescSets)
        {
            cmdBuf.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                **layout,
                0, staticDescriptorSets,
                {});
        }
    }

    auto getLayout() const noexcept -> const PipelineLayout&;

    void setStaticDescriptorSet(uint32_t setIndex, vk::DescriptorSet set) noexcept;

protected:
    Pipeline() = default;
    Pipeline(PipelineLayout& layout, vk::UniquePipeline pipeline)
        :
        layout(&layout),
        pipeline(std::move(pipeline))
    {}

private:
    PipelineLayout* layout;
    vk::UniquePipeline pipeline;

    std::vector<vk::DescriptorSet> staticDescriptorSets;
};

/**
 * @brief A graphics pipeline
 */
class GraphicsPipeline : public Pipeline,
                         public SelfManagedObject<GraphicsPipeline>
{
public:
    GraphicsPipeline() = default;
    GraphicsPipeline(vk::GraphicsPipelineCreateInfo info, PipelineLayout& layout)
        :
        Pipeline(
            layout,
            vkb::VulkanBase::getDevice()->createGraphicsPipelineUnique({}, info)
        )
    {
    }
};

/**
 * @brief A compute pipeline
 */
class ComputePipeline : public Pipeline,
                        public SelfManagedObject<ComputePipeline>
{
public:
    ComputePipeline(vk::PipelineShaderStageCreateInfo shader, const PipelineLayout& layout);
};
