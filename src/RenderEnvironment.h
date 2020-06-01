#pragma once
#ifndef RENDERENVIRONMENT_H
#define RENDERENVIRONMENT_H

#include <type_traits>

#include "vkb/VulkanBase.h"
#include "PipelineHelper.h"
#include "Framebuffer.h"
#include "RenderEngine.h"

class Renderpass;
class GraphicsPipeline;
class Scene;

struct RenderpassCreateInfo
{
    std::vector<vk::SubpassDescription> subpasses;
    std::vector<vk::SubpassDependency> dependencies;
    vk::RenderPassCreateFlags flags = {};
    std::vector<vk::DescriptorSet> renderpassLocalDescriptorSets;
};

class Renderpass : public vkb::VulkanManagedObject<Renderpass, const RenderpassCreateInfo&>
{
public:
    Renderpass(const RenderpassCreateInfo& info);
    Renderpass(const Renderpass&) = delete;
    Renderpass(Renderpass&&) = default;
    ~Renderpass();

    Renderpass& operator=(const Renderpass&) = delete;
    Renderpass& operator=(Renderpass&&) = default;

    inline auto get() const noexcept -> vk::RenderPass {
        return *renderpass;
    }
    inline auto operator*() const noexcept -> vk::RenderPass {
        return *renderpass;
    }

    auto getFramebuffer() const noexcept -> const Framebuffer&;
    auto getSubpassCount() const noexcept -> size_t;
    auto getRenderpassDescriptorSets() const noexcept -> const std::vector<vk::DescriptorSet>&;

    void addRenderpassDescriptorSet(vk::DescriptorSet descriptorSet) noexcept;

    void drawFrame(Scene& scene);

    /**
     * @brief Calls vk::CommandBuffer::beginRenderpass with the
     * appropriate arguments
     */
    void beginCommandBuffer(const vk::CommandBuffer& buf) const noexcept;

private:
    vk::UniqueRenderPass renderpass;
    Framebuffer framebuffer;
    const size_t subpassCount;

    // Frame synchronization
    vkb::FrameSpecificObject<vk::UniqueSemaphore> imageReadySemaphore;
    vkb::FrameSpecificObject<vk::UniqueSemaphore> renderFinishedSemaphore;
    vkb::FrameSpecificObject<vk::UniqueFence> duringRenderFence;

    // Renderpass-scoped resources
    std::vector<vk::DescriptorSet> staticDescriptorSets;

    RenderEngine engine;

    bool pauseRendering{ false };
};


/**
 * @brief A generic ck::Pipeline wrapper
 *
 * Specialized by pipeline subclasses, such as GraphicsPipeline
 */
class Pipeline
{
protected:
    Pipeline(vk::Pipeline pipeline, vk::PipelineLayout layout, vk::PipelineBindPoint bindPoint);

public:
    /**
     * The createGraphicsPipelineUnique doesnt work, idk why. So I have
     * to use the standard non-unique pipeline.
     */
    ~Pipeline();

    [[nodiscard]]
    auto get() noexcept -> vk::Pipeline&;

    [[nodiscard]]
    auto get() const noexcept -> const vk::Pipeline&;

    [[nodiscard]]
    auto operator*() const noexcept -> const vk::Pipeline&;

    [[nodiscard]]
    auto operator*() noexcept -> vk::Pipeline&;

    void bind(vk::CommandBuffer cmdBuf) const noexcept;

    [[nodiscard]]
    auto getPipelineLayout() const noexcept -> vk::PipelineLayout;

    [[nodiscard]]
    auto getPipelineDescriptorSets() const noexcept -> const std::vector<vk::DescriptorSet>&;

    void addPipelineDescriptorSet(vk::DescriptorSet descriptorSet) noexcept;

protected:
    vk::PipelineBindPoint bindPoint;
    vk::PipelineLayout layout;
    vk::Pipeline pipeline;

    std::vector<vk::DescriptorSet> staticDescriptorSets;
};


template<class Derived, size_t PipelineIndex, size_t SubpassIndex>
using has_graphics_pipeline = has_pipeline<Derived, GraphicsPipeline, PipelineIndex, SubpassIndex>;


struct GraphicsPipelineCreateInfo
{
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    vk::PipelineVertexInputStateCreateInfo vertexInput;
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    vk::PipelineTessellationStateCreateInfo tessellation;
    vk::PipelineViewportStateCreateInfo viewport;
    vk::PipelineRasterizationStateCreateInfo rasterization;
    vk::PipelineMultisampleStateCreateInfo multisample;
    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    vk::PipelineColorBlendStateCreateInfo colorBlend;
    vk::PipelineDynamicStateCreateInfo dynamicState;
    vk::PipelineLayout layout;
    const Renderpass& renderpass;
    uint32_t subpassIndex;
    vk::Pipeline basePipeline;
    int32_t basePipelineIndex;
    vk::PipelineCreateFlags flags;
};

/**
 * A vk::Pipeline RAII-wrapper.
 */
class GraphicsPipeline
    : public Pipeline,
      public vkb::VulkanManagedObject<GraphicsPipeline, const GraphicsPipelineCreateInfo&>
{
public:
    explicit GraphicsPipeline(const GraphicsPipelineCreateInfo& info);
    GraphicsPipeline(const GraphicsPipeline&) = default;
    GraphicsPipeline(GraphicsPipeline&&) = default;
    ~GraphicsPipeline() = default;

    GraphicsPipeline& operator=(const GraphicsPipeline&) = default;
    GraphicsPipeline& operator=(GraphicsPipeline&&) = default;
};

#endif
