#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "trc/Types.h"
#include "trc/VulkanInclude.h"

namespace trc
{
    class RenderPassRegistry;

    struct RenderPassInfo
    {
        vk::RenderPass pass;
        ui32 subpass;
    };

    struct DynamicRenderingInfo
    {
        ui32 viewMask{ 0x00 };
        std::vector<vk::Format> colorAttachmentFormats;
        vk::Format depthAttachmentFormat;
        vk::Format stencilAttachmentFormat;
    };

    /**
     * @brief Information about Vulkan's "render pass compatbility"
     *
     * Either a `RenderPassInfo`, which contains compatbility information in
     * form of a render pass and a specific subpass for the traditional render
     * pass based rendering, or `DynamicRenderingInfo`, which contains
     * sufficient compatibility information for `VK_KHR_dynamic_rendering`.
     */
    using RenderPassCompatInfo = std::variant<RenderPassInfo, DynamicRenderingInfo>;

    /**
     * @brief Reference to a render pass
     *
     * Is registered at a `RenderPassRegistry` as a centralized storage for
     * render pass compatibility information.
     */
    using RenderPassName = std::string;

    /**
     * @brief Any kind of information that is sufficient to define render pass
     *        compatibility.
     *
     * Either a reference to a render pass, which has to be resolved via a
     * `RenderPassRegistry`, or an inline object with that information.
     */
    using RenderPassDefinition = std::variant<RenderPassName, RenderPassCompatInfo>;

    /**
     * @brief Resolve a possibly-reference to a concrete structure
     */
    auto resolveRenderPass(const RenderPassDefinition& def,
                           const RenderPassRegistry& reg)
        -> RenderPassCompatInfo;

    /**
     * @brief A mapping that contains render passes for the only purpose of
     *        providing information about render pass compatibility to where
     *        Vulkan requires it (i.e. pipeline creation).
     *
     * Render pass information can be either a render pass object or attachment
     * information for `VK_KHR_dynamic_rendering` (core in Vulkan 1.3).
     */
    class RenderPassRegistry
    {
    public:
        using RenderPassGetter = std::function<RenderPassCompatInfo()>;

        /**
         * @brief Register a render pass for compatibility information
         */
        void addRenderPass(const RenderPassName& name, vk::RenderPass renderPass, ui32 subPass);

        /**
         * @brief Register a renderpassless dynamic rendering context
         */
        void addRenderPass(const RenderPassName& name, DynamicRenderingInfo info);

        /**
         * @param RenderPassGetter getter Function for lazy access to the render
         *        pass, in case it is created conditionally or at a later point.
         *        Can return either a render pass or dynamic rendering info.
         */
        void addRenderPass(const RenderPassName& name, const RenderPassGetter& getter);

        /**
         * @throw Exception if no render pass with `name` is registered.
         */
        auto getRenderPass(const RenderPassName& name) const
            -> RenderPassCompatInfo;

    private:
        std::unordered_map<std::string, RenderPassGetter> renderPasses;
    };
} // namespace trc
