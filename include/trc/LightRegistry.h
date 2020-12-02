#pragma once

#include "Light.h"
#include "Node.h"
#include "RenderPassShadow.h"
#include "TorchResources.h"

namespace trc
{
    class LightRegistry;

    class ShadowDescriptor
    {
        friend class LightRegistry;

    public:
        static constexpr ui32 MAX_SHADOW_MAPS = 256;

        explicit ShadowDescriptor(const LightRegistry &lightRegistry, ui32 numShadowMaps);

        auto getProvider() const noexcept -> const DescriptorProviderInterface&;

        /**
         * The descriptor set layout is global for all SceneDescriptor
         * instances.
         */
        static auto getDescLayout() noexcept -> vk::DescriptorSetLayout;

    private:
        // The descriptor set layout is the same for all instances
        static inline vk::UniqueDescriptorSetLayout descLayout;
        static inline vkb::Image dummyShadowImage;
        static inline vk::UniqueImageView dummyImageView;
        static vkb::StaticInit _init;

        void createDescriptors(const LightRegistry& lightRegistry, ui32 numShadowMaps);
        vk::UniqueDescriptorPool descPool;
        vkb::FrameSpecificObject<vk::UniqueDescriptorSet> descSets;
        FrameSpecificDescriptorProvider provider{ *descLayout, {} };
    };

    constexpr ui32 DEFAULT_MAX_LIGHTS = 32;

    /**
     * @brief Collection and management unit for lights and shadows
     */
    class LightRegistry
    {
    public:
        explicit LightRegistry(ui32 maxLights = DEFAULT_MAX_LIGHTS);

        /**
         * @brief Update lights in the registry
         *
         * Updates the light buffer. Applies transformations of attached
         * light nodes to their lights.
         */
        void update();

        auto getDescriptor() const noexcept -> const ShadowDescriptor&;

        ui32 getMaxLights() const noexcept;

        /**
         * @return const Light& The added light
         */
        auto addLight(Light& light) -> Light&;

        /**
         * Also removes a light node that has the light attached if such
         * a node exists.
         */
        void removeLight(const Light& light);

        /**
         * @brief Storage of and handle to a light's shadow
         *
         * A shadow consists of one (e.g. sun lights) or more (e.g. point
         * lights) cameras that define the shadow direction and projection.
         * The handle gives limited access to these cameras.
         */
        struct ShadowInfo
        {
            /**
             * @return Node& A node that all shadow cameras are attached to
             */
            auto getNode() noexcept -> Node&;

            /**
             * @brief Set a projection matrix on all shadow cameras
             */
            void setProjectionMatrix(mat4 proj) noexcept;

            auto getRenderPasses() const noexcept -> const std::vector<RenderPassShadow*>&;

        private:
            friend LightRegistry;

            std::vector<RenderPassShadow*> shadowPasses;
            std::vector<Camera> shadowCameras;
            Node parentNode;
        };


        /**
         * @brief Enable shadows for a specific light
         *
         * In order to work properly, a position should be set on sun
         * lights before passing them to this function.
         *
         * @param Light& light      The light that shall cast shadows.
         * @param uvec2  resolution The resolution of the created shadow
         *                          map. This can not be changed later on.
         * @param ShadowStage& renderStage The stage that created render
         *                                 passes shall be attached to.
         *
         * @return ShadowInfo&
         *
         * @throw std::invalid_argument if shadows are already enabled on the light
         * @throw std::runtime_error if something unexpected happens
         */
        auto enableShadow(Light& light, uvec2 shadowResolution) -> ShadowInfo&;

        /**
         * Does nothing if shadows are not enabled for the light
         */
        void disableShadow(Light& light);

        auto getLightBuffer() const noexcept -> vk::Buffer;
        auto getShadowMatrixBuffer() const noexcept -> vk::Buffer;
        auto getShadowRenderStage() const noexcept -> const std::vector<RenderPass::ID>&;

    private:
        const ui32 maxLights;
        const ui32 maxShadowMaps;

        // Must be done every frame in case light properties change
        void updateLightBuffer();
        std::vector<Light*> lights;
        vkb::Buffer lightBuffer;

        std::vector<RenderPass::ID> shadowPasses;

        /**
         * Must be called only when a light or a shadow is added or removed
         *
         * Re-orders shadow matrices and -maps according to the order
         * of lights in the light array. This means it also sets the
         * correct shadow indices on the lights.
         */
        void updateShadowDescriptors();

        /**
         * This must be called every frame in case a shadow matrix changes
         */
        void updateShadowMatrixBuffer();
        std::unordered_map<Light*, ShadowInfo> shadows;
        vkb::Buffer shadowMatrixBuffer;
        ui32 nextFreeShadowPassIndex; // Initial value set in constructor because recursive include
        std::vector<ui32> freeShadowPassIndices;

        std::unique_ptr<ShadowDescriptor> shadowDescriptor;
    };
} // namespace trc
