#include "trc/Light.h"

#include <trc_util/Assert.h>



namespace trc
{

auto impl::LightInterfaceBase::linkShadowMap(const ui32 shadowMapIndex) -> bool
{
    if (data->numShadowMaps >= data->MAX_SHADOW_MAPS) {
        return false;
    }

    // Link the shadow map to the light
    data->shadowMapIndices[data->numShadowMaps] = shadowMapIndex;
    ++data->numShadowMaps;

    return true;
}

void impl::LightInterfaceBase::clearShadowMaps()
{
    data->numShadowMaps = 0;
}

auto impl::ColoredLightInterface::getColor() const -> vec3
{
    return vec3{ data->color };
}

auto impl::ColoredLightInterface::getAmbientPercentage() const -> float
{
    return data->ambientPercentage;
}

void impl::ColoredLightInterface::setColor(vec3 newColor)
{
    data->color = vec4{ newColor, 1.0f };
}

void impl::ColoredLightInterface::setAmbientPercentage(float ambient)
{
    data->ambientPercentage = ambient;
}

auto SunLightInterface::getDirection() const -> vec3
{
    return vec3{ data->direction };
}

void SunLightInterface::setDirection(vec3 newDir)
{
    data->direction = vec4{ newDir, 0.0f };
}

auto PointLightInterface::getPosition() const -> vec3
{
    return vec3{ data->position };
}

void PointLightInterface::setPosition(vec3 newPos)
{
    data->position = vec4{ newPos, 1.0f };
}

} // namespace trc
