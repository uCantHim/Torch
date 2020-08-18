#pragma once

#include <vector>
#include <atomic>

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

    /**
     * @brief The render stage that renders shadow maps
     */
    class ShadowStage : public RenderStage
    {
    public:
        ShadowStage() : RenderStage(1) {}

        //void addRenderPass(RenderPass::ID newPass) final;
        //void removeRenderPass(RenderPass::ID pass) final;
    };

    /**
     * @brief A pass that renders a shadow map
     */
    class RenderPassShadow : public RenderPass, public Node
    {
    public:
        /**
         * Enables shadows on the light.
         *
         * @param uvec2 resolution Resolution of the shadow map
         * @param mat4 projMatrix Projection matrix of the shadow map
         */
        RenderPassShadow(uvec2 resolution, const mat4& projMatrix, Light& light);
        ~RenderPassShadow() override;

        /**
         * Updates the shadow matrix in the descriptor and starts the
         * renderpass.
         */
        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents) override;
        void end(vk::CommandBuffer cmdBuf) override;

        auto getResolution() const noexcept -> uvec2;
        auto getProjectionMatrix() const noexcept -> const mat4&;
        void setProjectionMatrix(const mat4& view) noexcept;

        auto getShadowIndex() const noexcept -> ui32;

    private:
        Light* light;

        uvec2 resolution;
        mat4 projMatrix;
        ui32 shadowDescriptorIndex;

        vkb::FrameSpecificObject<vkb::Image> depthImages;
        vkb::FrameSpecificObject<vk::UniqueImageView> depthImageViews;
        vkb::FrameSpecificObject<vk::UniqueFramebuffer> framebuffers;
    };

    class ShadowDescriptor
    {
    public:
        static auto getProvider() noexcept -> const DescriptorProviderInterface&;

        static auto addShadow(const vkb::FrameSpecificObject<vk::Sampler>& samplers,
                              const vkb::FrameSpecificObject<vk::ImageView>& views,
                              const mat4& viewProjMatrix) -> ui32;
        static void updateShadow(ui32 shadowIndex, const mat4& viewProjMatrix);
        static void removeShadow(ui32 index);

    private:
        static auto getNewIndex() -> ui32;
        static inline std::atomic<ui32> nextIndex{ 0 };
        static inline std::vector<ui32> freeIndices;

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
