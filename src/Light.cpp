#include "Light.h"



auto trc::makeSunLight(vec3 color, vec3 direction, float ambientPercent) -> Light
{
    return {
        .color                = vec4(color, 1.0f),
        .position             = vec4(0.0f),
        .direction            = vec4(direction, 0.0f),
        .ambientPercentage    = ambientPercent,
        .attenuationLinear    = 0.0f,
        .attenuationQuadratic = 0.0f,
        .type                 = Light::Type::eSunLight
    };
}

auto trc::makePointLight(vec3 color,
                         vec3 position,
                         float attLinear,
                         float attQuadratic) -> Light
{
    return {
        .color                = vec4(color, 1.0f),
        .position             = vec4(position, 1.0f),
        .direction            = vec4(0.0f),
        .ambientPercentage    = 0.0f,
        .attenuationLinear    = attLinear,
        .attenuationQuadratic = attQuadratic,
        .type                 = Light::Type::ePointLight
    };
}

auto trc::makeAmbientLight(vec3 color) -> Light
{
    return {
        .color                = vec4(color, 1.0f),
        .position             = vec4(0.0f),
        .direction            = vec4(0.0f),
        .ambientPercentage    = 1.0f,
        .attenuationLinear    = 0.0f,
        .attenuationQuadratic = 0.0f,
        .type                 = Light::Type::eAmbientLight
    };
}
