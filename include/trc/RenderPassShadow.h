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
     * @brief Enable shadows for a light
     *
     * Creates one or more (depending on the type of light) shadow passes
     * for a light. Registers the pass(es) at the ShadowStage. Enables
     * shadows on the light. Long story short, no further work is required
     * after calling this function.
     *
     * @param Light& light      The light that shall cast shadows.
     * @param uvec2  resolution The resolution of the created shadow map.
     * @param mat4   projMatrix A projection matrix for the created
     *                          renderpass.
     *
     * @return Node A node that all created shadow passes are attached to.
     *              This node could be attached to a light node of the
     *              light, for example.
     */
    extern auto enableShadow(Light& light, uvec2 shadowMapResolution, mat4 projectionMatrix)
        -> Node;

    /**
     * @brief The render stage that renders shadow maps
     */
    class ShadowStage : public RenderStage
    {
    public:
        ShadowStage() : RenderStage(1) {}
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
        RenderPassShadow(uvec2 resolution, const mat4& projMatrix);
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

        /**
         * @return ui32 The pass's index in all shadow-related resources
         *              on the device. This includes the shadow matrix
         *              buffer and the array of shadow map samplers.
         */
        auto getShadowIndex() const noexcept -> ui32;

    private:
        uvec2 resolution;
        mat4 projMatrix;
        ui32 shadowDescriptorIndex;

        vkb::FrameSpecificObject<vkb::Image> depthImages;
        vkb::FrameSpecificObject<vk::UniqueImageView> depthImageViews;
        vkb::FrameSpecificObject<vk::UniqueFramebuffer> framebuffers;
    };

    struct ShadowDescriptorInfo
    {
        const vkb::FrameSpecificObject<vk::Sampler>& samplers;
        const vkb::FrameSpecificObject<vk::ImageView>& views;
        mat4 viewProjMatrix;
    };

    class ShadowDescriptor
    {
    public:
        static auto getProvider() noexcept -> const DescriptorProviderInterface&;

        /**
         * @brief Add information for one shadow to the descriptor
         */
        static auto addShadow(const ShadowDescriptorInfo& shadowInfo) -> ui32;

        /**
         * @brief Update the view-projection matrix for a shadow
         *
         * @param ui32 shadowIndex Index of the shadow to update
         * @param const mat4& viewProjMatrix New shadow matrix
         */
        static void updateShadow(ui32 shadowIndex, const mat4& viewProjMatrix);

        /**
         * @brief Remove a shadow from the descriptor
         *
         * Does not modify any memory or descriptor bindings, just frees
         * the index.
         */
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
