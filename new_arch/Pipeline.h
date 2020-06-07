#pragma once

#include <vulkan/vulkan.hpp>

#include "SelfManagedObject.h"

class PipelineLayout
{
public:
    auto operator*();
    auto get();

    void addDescriptorSetLayouts();
    auto getDescriptorSetLayouts();

    void addPushConstantRanges();
    auto getPushConstantRanges();

private:
    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
    std::vector<vk::PushConstantRange> pushConstantRanges;

    vk::UniquePipelineLayout layout;
};

/**
 * @brief Base class for all pipelines
 */
class Pipeline
{
public:
    Pipeline(Pipeline&&) noexcept = default;

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline& operator=(Pipeline&&) noexcept = delete;

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
    void bind(vk::CommandBuffer cmdBuf, bool bindStaticDescSets = true) const noexcept;

    auto getLayout() const noexcept -> const PipelineLayout&;

    void setStaticDescriptorSet(uint32_t setIndex, vk::DescriptorSet set) noexcept;

protected:
    Pipeline(const PipelineLayout& layout, vk::UniquePipeline pipeline);

private:
    const PipelineLayout& layout;
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
    GraphicsPipeline(vk::GraphicsPipelineCreateInfo info, const PipelineLayout& layout);
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
