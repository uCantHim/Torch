#pragma once

#include <vulkan/vulkan.hpp>

#include "RenderEnvironment.h"

/**
 * @brief A polymorphic interface for drawables
 */
class VulkanDrawableInterface
{
public:
    /**
     * Records the secondary command buffer that draws the drawable
     */
    virtual auto recordCommandBuffer(
        uint32_t subpass,
        const vk::CommandBufferInheritanceInfo& inheritInfo
    ) -> vk::CommandBuffer = 0;

    virtual auto getPipeline(uint32_t subpass) const -> GraphicsPipeline& = 0;
};

/**
 * @brief A CRTP base class for all drawables
 *
 * Implementations of drawables should inherit from this class. A specific
 * drawable class is always associated with a specific renderpass.
 */
template<class Derived>
class VulkanDrawable : public VulkanDrawableInterface,
                       private internal::pipeline_helper<Derived, GraphicsPipeline>
{
public:
    VulkanDrawable() = default;

    auto getPipeline(uint32_t subpass) const -> GraphicsPipeline& final {
        return internal::pipeline_helper<Derived, GraphicsPipeline>::_get_pipeline(subpass);
    }
};
