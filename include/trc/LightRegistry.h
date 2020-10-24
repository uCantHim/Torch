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

        explicit _ShadowDescriptor(const LightRegistry& lightRegistry, ui32 maxShadowMaps);

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
        static vkb::StaticInit _init;

        void createDescriptors(const LightRegistry& lightRegistry, ui32 maxShadowMaps);
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

        auto getLightBuffer() const noexcept -> vk::Buffer;
        auto getShadowMatrixBuffer() const noexcept -> vk::Buffer;

    private:
        const ui32 maxLights;
        const ui32 maxShadowMaps;

        // Must be done every frame in case light properties change
        void updateLightBuffer();
        std::vector<Light*> lights;
        vkb::Buffer lightBuffer;

        struct ShadowInfo
        {
            // Has images
            const RenderPassShadow& shadowPass;
            Camera shadowCamera;
        };
        // Must be done only when a light is added or removed
        void updateShadowDescriptors();
        // This on the other hand must be done every frame in case the view-proj changes
        void updateShadowMatrixBuffer();
        std::unordered_map<const Light*, ShadowInfo> shadows;
        vkb::Buffer shadowMatrixBuffer;

        _ShadowDescriptor shadowDescriptor;

        /**
         * The light pointer allows me to delete light nodes when I only
         * have a corresponding light, as is the case in removeLight().
         */
        std::vector<std::pair<const Light*, std::unique_ptr<LightNode>>> lightNodes;
    };
} // namespace trc
