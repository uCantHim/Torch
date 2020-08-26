#pragma once

#include <vkb/Buffer.h>

#include "Boilerplate.h"
#include "Node.h"

namespace trc
{
    struct Light
    {
        enum Type : ui32
        {
            eSunLight = 0,
            ePointLight = 1,
            eAmbientLight = 2,
        };

        vec4 color{ 1.0f };
        vec4 position{ 0.0f };
        vec4 direction{ 1.0f };

        float ambientPercentage{ 0.0f };
        float attenuationLinear{ 0.5f };
        float attenuationQuadratic{ 0.0f };

        Type type;

        bool32 hasShadow{ false };
        ui32 firstShadowIndex{ 0 };

        ui32 __padding[2]{ 0, 0 };
    };

    constexpr ui32 MAX_LIGHTS = 32;

    /**
     * @brief Create a sunlight
     */
    extern auto makeSunLight(vec3 color, vec3 direction, float ambientPercent = 0.0f) -> Light;

    /**
     * @brief Create a pointlight
     */
    extern auto makePointLight(vec3 color,
                               vec3 position,
                               float attLinear = 0.0f,
                               float attQuadratic = 0.0f) -> Light;

    /**
     * @brief Create an ambientlight
     */
    extern auto makeAmbientLight(vec3 color) -> Light;

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
        void update();

    private:
        Light* light;
        vec4 initialDirection;
    };

    /**
     * @brief Collection and management unit for lights and shadows
     */
    class LightRegistry
    {
    public:
        LightRegistry();

        /**
         * @brief Update lights in the registry
         *
         * Updates the light buffer.
         */
        void update();

        /**
         * @return const Light& The added light
         */
        auto addLight(const Light& light) -> const Light&;

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

    private:
        void updateLightBuffer();
        std::vector<const Light*> lights;
        vkb::Buffer lightBuffer;

        /**
         * The light pointer allows me to delete light nodes when I only
         * have a corresponding light, as is the case in removeLight().
         */
        std::vector<std::pair<const Light*, std::unique_ptr<LightNode>>> lightNodes;
    };
} // namespace trc
