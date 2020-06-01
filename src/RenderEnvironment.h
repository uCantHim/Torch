#pragma once

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

class Renderpass : public vkb::VulkanManagedObject<Renderpass>
{
public:
    explicit Renderpass(const RenderpassCreateInfo& info);

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
 * @brief A pipeline layout
 */
class PipelineLayout
{
public:
    /**
     * @brief Create the layout in an uninitialized state
     *
     * Does not allocate any Vulkan resources.
     */
    PipelineLayout() = default;

    /**
     * @brief Create a pipeline layout
     */
    PipelineLayout(std::vector<vk::DescriptorSetLayout> descriptorSets,
                   std::vector<vk::PushConstantRange> pushConstants);

    auto operator*() const noexcept -> vk::PipelineLayout;
    auto get() const noexcept -> vk::PipelineLayout;

    auto getDescriptorSetLayout(uint32_t setIndex) -> vk::DescriptorSetLayout;
    auto getDescriptorSetLayouts() const noexcept -> const std::vector<vk::DescriptorSetLayout>&;

    auto getPushConstantRange(uint32_t pushConstantIndex) -> vk::PushConstantRange;
    auto getPushConstantRanges() const noexcept -> const std::vector<vk::PushConstantRange>&;

private:
    vk::UniquePipelineLayout layout;

    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
    std::vector<vk::PushConstantRange> pushConstantRanges;
};

/**
 * @brief A graphics pipeline
 */
class GraphicsPipeline : public vkb::VulkanManagedObject<GraphicsPipeline>
{
public:
    GraphicsPipeline(const PipelineLayout& layout, vk::GraphicsPipelineCreateInfo createInfo);
    ~GraphicsPipeline() = default;

    auto operator*() const noexcept -> vk::Pipeline;
    auto get() const noexcept -> vk::Pipeline;

    /**
     * @brief Bind the pipeline
     *
     * Also binds all static descriptor sets of the pipeline.
     *
     * @param vk::CommandBuffer cmdBuf The command buffer to record the
     *                                 pipeline bind to.
     */
    void bind(vk::CommandBuffer cmdBuf) const noexcept;

    auto getLayout() const noexcept -> const PipelineLayout&;

    void setStaticDescriptorSet(uint32_t setIndex, vk::DescriptorSet descriptorSet) noexcept;

protected:
    const PipelineLayout& layout;
    vk::UniquePipeline pipeline;

    std::vector<std::pair<uint32_t, vk::DescriptorSet>> staticDescriptorSets;
};


template<class Derived, size_t PipelineIndex, size_t SubpassIndex>
using has_graphics_pipeline = has_pipeline<Derived, GraphicsPipeline, PipelineIndex, SubpassIndex>;
