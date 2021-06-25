#pragma once

#include <unordered_map>

#include <vkb/Buffer.h>
#include <Camera.h>

#include "Types.h"

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

    /**
     * @return Number of shadow maps based on light type
     * @throw std::logic_error if the enum doesn't exist
     */
    extern ui32 getNumShadowMaps(const Light& light);

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
} // namespace trc
