#pragma once

#include "Light.h"

namespace trc
{
    /**
     * @brief A node with an attached light
     */
    class LightNode : public Node
    {
    public:
        LightNode(Light& light);

        /**
         * @brief Update the light's position
         *
         * Applies the node's transformation to its light
         */
        void applyTransformToLight();

    private:
        Light* light;
        vec4 initialDirection;
    };

    class LightRegistry;

    class _ShadowDescriptor
    {
    public:
        static constexpr ui32 MAX_SHADOW_MAPS = 256;

        explicit _ShadowDescriptor(const LightRegistry& lightRegistry, ui32 numShadowMaps);

        /**
         * The descriptor set is per-scene
         */
        auto getDescSet(ui32 imageIndex) const noexcept -> vk::DescriptorSet;

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
    };

    constexpr ui32 DEFAULT_MAX_LIGHTS = 32;

    /**
     * @brief Collection and management unit for lights and shadows
     */
    class LightRegistry
    {
    public:
        struct LightHandle
        {
        private:
            friend LightRegistry;
            ui32 lightIndex;
        };

        explicit LightRegistry(ui32 maxLights = DEFAULT_MAX_LIGHTS);

        /**
         * @brief Update lights in the registry
         *
         * Updates the light buffer. Applies transformations of attached
         * light nodes to their lights.
         */
        void update();

        auto getDescriptor() const noexcept -> const _ShadowDescriptor&;

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
         * @brief Create a light node for a light
         *
         * LightRegistry::update updates the created node.
         *
         * @param Light& light The light attached to the node
         *
         * @return LightNode& The created node
         */
        auto createLightNode(Light& light) -> LightNode&;

        /**
         * @brief Remove a light node from the registry
         */
        void removeLightNode(const LightNode& node);

        /**
         * @brief Remove a light node that has a specific light attached
         *
         * Does nothing if no node with the passed light exists.
         */
        void removeLightNode(const Light& light);

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

        private:
            friend LightRegistry;

            ShadowStage* shadowStage;
            std::vector<RenderPassShadow*> shadowPasses;
            std::vector<Camera> shadowCameras;
            Node parentNode;
        };


        /**
         * In order to work properly, a position should be set on sun
         * lights before passing them to this function.
         *
         * @throw std::invalid_argument if shadows are already enabled on the light
         * @throw std::runtime_error if something unexpected happens
         */
        auto enableShadow(Light& light,
                          uvec2 shadowResolution,
                          ShadowStage& renderStage) -> ShadowInfo&;

        /**
         * Does nothing if shadows are not enabled for the light
         */
        void disableShadow(Light& light);

        auto getLightBuffer() const noexcept -> vk::Buffer;
        auto getShadowMatrixBuffer() const noexcept -> vk::Buffer;

    private:
        const ui32 maxLights;
        const ui32 maxShadowMaps;

        // Must be done every frame in case light properties change
        void updateLightBuffer();
        std::vector<Light*> lights;
        vkb::Buffer lightBuffer;

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

        std::unique_ptr<_ShadowDescriptor> shadowDescriptor;

        /**
         * The light pointer allows me to delete light nodes when I only
         * have a corresponding light, as is the case in removeLight().
         */
        std::vector<std::pair<const Light*, std::unique_ptr<LightNode>>> lightNodes;
    };
} // namespace trc
