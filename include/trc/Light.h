#pragma once

#include <vkb/Buffer.h>

#include "Boilerplate.h"

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

        vec4 color;
        vec4 position;
        vec4 direction;

        float ambientPercentage{ 0.0f };
        float attenuationLinear{ 0.5f };
        float attenuationQuadratic{ 0.0f };

        Type type;
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

        void addLight(const Light& light);
        void removeLight(const Light& light);

        auto getLightBuffer() const noexcept -> vk::Buffer;

    private:
        void updateLightBuffer();
        std::vector<const Light*> lights;
        vkb::Buffer lightBuffer;
    };
} // namespace trc
