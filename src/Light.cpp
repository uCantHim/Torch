#include "Light.h"

#include "utils/Util.h"



trc::Light::Light(LightData& lightData)
    :
    data(&lightData)
{
}

trc::Light::operator bool() const
{
    return data != nullptr;
}

auto trc::Light::getType() const -> Type
{
    assert(*this);
    return data->type;
}

auto trc::Light::getColor() const -> vec3
{
    assert(*this);
    return data->color;
}

auto trc::Light::getPosition() const -> vec3
{
    assert(*this);
    return data->position;
}

auto trc::Light::getDirection() const -> vec3
{
    assert(*this);
    return data->direction;
}

auto trc::Light::getAmbientPercentage() const -> float
{
    assert(*this);
    return data->ambientPercentage;
}

void trc::Light::setColor(vec3 newColor)
{
    assert(*this);
    data->color = vec4(newColor, 1.0f);
}

void trc::Light::setPosition(vec3 newPos)
{
    assert(*this);
    data->position = vec4(newPos, 1.0f);
}

void trc::Light::setDirection(vec3 newDir)
{
    assert(*this);
    data->direction = vec4(newDir, 0.0f);
}

void trc::Light::setAmbientPercentage(float ambient)
{
    assert(*this);
    data->ambientPercentage = ambient;
}

void trc::Light::addShadowMap(const ui32 shadowMapIndex)
{
    assert(*this);
    assert(data->numShadowMaps < LightData::MAX_SHADOW_MAPS);

    data->shadowMapIndices[data->numShadowMaps] = shadowMapIndex;
    data->numShadowMaps++;
}

void trc::Light::removeAllShadowMaps()
{
    assert(*this);

    data->numShadowMaps = 0;
    for (ui32 i = 0; i < LightData::MAX_SHADOW_MAPS; i++) {
        data->shadowMapIndices[i] = 0;
    }
}



auto trc::getNumShadowMaps(const Light::Type lightType) -> ui32
{
    switch (lightType)
    {
    case LightData::Type::eSunLight:
        return 1;
    case LightData::Type::ePointLight:
        return 4;
    case LightData::Type::eAmbientLight:
        return 0;
    default:
        throw std::logic_error("Light type enum does not exist");
    }
}
