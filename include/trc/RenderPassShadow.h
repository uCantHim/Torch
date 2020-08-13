#pragma once

#include <vector>

#include <vkb/Buffer.h>
#include <vkb/Image.h>

#include "Boilerplate.h"
#include "Node.h"
#include "RenderStage.h"
#include "RenderPass.h"
#include "Light.h"

namespace trc
{
    constexpr ui32 MAX_SHADOW_MAPS = MAX_LIGHTS * 4;

    /** Forward declaration */
    struct Light;

    /**
     * @brief Base class of all shadow passes
     *
     * For additional type safety.
     */
    class RenderPassShadow : public RenderPass
    {
    public:
        RenderPassShadow(vk::UniqueRenderPass renderPass, ui32 subpassCount)
            : RenderPass(std::move(renderPass), subpassCount)
        {}
    };

    /**
     * @brief Renderpass for a sun shadow
     */
    class SunShadowPass : public RenderPassShadow
    {
    public:
        SunShadowPass(uvec2 resolution, const Light& light);

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents) override;
        void end(vk::CommandBuffer cmdBuf) override;

        auto getResolution() const noexcept -> uvec2;

    private:
        uvec2 resolution;
        const Light* light;

        vkb::FrameSpecificObject<vkb::Image> depthImages;
        vkb::FrameSpecificObject<vk::UniqueImageView> depthImageViews;
        vkb::FrameSpecificObject<vk::UniqueFramebuffer> framebuffers;
    };

    /**
     * @brief The render stage that renders shadow maps
     */
    class ShadowStage : public RenderStage
    {
    public:
        //void addRenderPass(RenderPass::ID newPass) final;
        //void removeRenderPass(RenderPass::ID pass) final;
    };

    class ShadowDescriptor
    {
    public:
        static auto getProvider() noexcept -> const DescriptorProviderInterface&;

        static void addShadow(vk::Image image, const mat4& viewMatrix, const mat4& projMatrix);

    private:
        static inline vkb::Buffer shadowMatrixBuffer;

        static inline vk::UniqueDescriptorPool descPool;
        static inline vk::UniqueDescriptorSetLayout descLayout;
        static inline std::unique_ptr<vkb::FrameSpecificObject<vk::UniqueDescriptorSet>> descSet;
        static inline std::unique_ptr<FrameSpecificDescriptorProvider> descProvider{ nullptr };

        static void init();
        static void updateDescriptorSet();
        static void destroy();
        static inline vkb::StaticInit _init{ init, destroy };
    };
} // namespace trc
