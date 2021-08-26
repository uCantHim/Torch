#include "LightRegistry.h"



auto trc::LightRegistry::makeSunLight(vec3 color, vec3 direction, float ambientPercent)
    -> Light
{
    return addLight({
        .color                = vec4(color, 1.0f),
        .position             = vec4(0.0f),
        .direction            = vec4(direction, 0.0f),
        .ambientPercentage    = ambientPercent,
        .attenuationLinear    = 0.0f,
        .attenuationQuadratic = 0.0f,
        .type                 = LightData::Type::eSunLight
    });
}

auto trc::LightRegistry::makeSunLightUnique(vec3 color, vec3 direction, float ambientPercent)
    -> UniqueLight
{
    return UniqueLight(
        new Light(makeSunLight(color, direction, ambientPercent)),
        [this](Light* light) {
            deleteLight(*light);
            delete light;
        }
    );
}

auto trc::LightRegistry::makePointLight(
    vec3 color,
    vec3 position,
    float attLinear,
    float attQuadratic) -> Light
{
    return addLight({
        .color                = vec4(color, 1.0f),
        .position             = vec4(position, 1.0f),
        .direction            = vec4(0.0f),
        .ambientPercentage    = 0.0f,
        .attenuationLinear    = attLinear,
        .attenuationQuadratic = attQuadratic,
        .type                 = LightData::Type::ePointLight
    });
}

auto trc::LightRegistry::makePointLightUnique(
    vec3 color,
    vec3 position,
    float attLinear,
    float attQuadratic) -> UniqueLight
{
    return UniqueLight(
        new Light(makePointLight(color, position, attLinear, attQuadratic)),
        [this](Light* light) {
            deleteLight(*light);
            delete light;
        }
    );
}

auto trc::LightRegistry::makeAmbientLight(vec3 color) -> Light
{
    return addLight({
        .color                = vec4(color, 1.0f),
        .position             = vec4(0.0f),
        .direction            = vec4(0.0f),
        .ambientPercentage    = 1.0f,
        .attenuationLinear    = 0.0f,
        .attenuationQuadratic = 0.0f,
        .type                 = LightData::Type::eAmbientLight
    });
}

auto trc::LightRegistry::makeAmbientLightUnique(vec3 color) -> UniqueLight
{
    return UniqueLight(
        new Light(makeAmbientLight(color)),
        [this](Light* light) {
            deleteLight(*light);
            delete light;
        }
    );
}

auto trc::LightRegistry::addLight(LightData light) -> Light
{
    requiredLightDataSize += sizeof(LightData);

    switch (light.type)
    {
    case LightData::Type::eSunLight:
        return Light(*sunLights.emplace_back(new LightData(light)));
    case LightData::Type::ePointLight:
        return Light(*pointLights.emplace_back(new LightData(light)));
    case LightData::Type::eAmbientLight:
        return Light(*ambientLights.emplace_back(new LightData(light)));
    }

    // This should not be able to happen
    throw std::logic_error("Light type \"" + std::to_string(light.type) + "\" does not exist");
}

void trc::LightRegistry::deleteLight(Light light)
{
    if (!light) return;

    auto remove = [this](std::vector<u_ptr<LightData>>& lights, LightData* light) {
        auto it = std::remove_if(lights.begin(), lights.end(),
                                 [&](auto& l) { return l.get() == light; });
        if (it != lights.end())
        {
            lights.erase(it);
            requiredLightDataSize -= sizeof(LightData);
        }
    };

    switch (light.data->type)
    {
    case LightData::Type::eSunLight:
        remove(sunLights, light.data);
        break;
    case LightData::Type::ePointLight:
        remove(pointLights, light.data);
        break;
    case LightData::Type::eAmbientLight:
        remove(ambientLights, light.data);
        break;
    }
}

bool trc::LightRegistry::lightExists(Light light)
{
    if (!light) return false;

    auto compare = [&](auto& ptr) { return light.data == ptr.get(); };
    switch (light.data->type)
    {
    case LightData::Type::eSunLight:
        return std::find_if(sunLights.begin(), sunLights.end(), compare) == sunLights.end();
    case LightData::Type::ePointLight:
        return std::find_if(pointLights.begin(), pointLights.end(), compare) == pointLights.end();
    case LightData::Type::eAmbientLight:
        return std::find_if(ambientLights.begin(), ambientLights.end(), compare) == ambientLights.end();
    }

    throw std::logic_error("Light type \"" + std::to_string(light.data->type) + "\" exists");
}

auto trc::LightRegistry::getRequiredLightDataSize() const -> ui32
{
    return requiredLightDataSize;
}

void trc::LightRegistry::writeLightData(ui8* buf) const
{
    // Set number of lights
    const ui32 numSunLights = sunLights.size();
    const ui32 numPointLights = pointLights.size();
    const ui32 numAmbientLights = ambientLights.size();
    memcpy(buf,                    &numSunLights,     sizeof(ui32));
    memcpy(buf + sizeof(ui32),     &numPointLights,   sizeof(ui32));
    memcpy(buf + sizeof(ui32) * 2, &numAmbientLights, sizeof(ui32));

    // Copy light data
    size_t offset = sizeof(vec4);
    for (const auto& light : sunLights)
    {
        memcpy(buf + offset, &*light, sizeof(LightData));
        offset += util::sizeof_pad_16_v<LightData>;
    }
    for (const auto& light : pointLights)
    {
        memcpy(buf + offset, &*light, sizeof(LightData));
        offset += util::sizeof_pad_16_v<LightData>;
    }
    for (const auto& light : ambientLights)
    {
        memcpy(buf + offset, &*light, sizeof(LightData));
        offset += util::sizeof_pad_16_v<LightData>;
    }
}
