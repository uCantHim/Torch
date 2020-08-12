#include "Light.h"

#include "utils/Util.h"



trc::LightRegistry::LightRegistry()
    :
    lightBuffer(
        util::sizeof_pad_16_v<Light> * MAX_LIGHTS,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    )
{
}

void trc::LightRegistry::update()
{
    updateLightBuffer();
}

void trc::LightRegistry::addLight(const Light& light)
{
    lights.push_back(&light);
}

void trc::LightRegistry::removeLight(const Light& light)
{
    lights.erase(std::find(lights.begin(), lights.end(), &light));
}

auto trc::LightRegistry::getLightBuffer() const noexcept -> vk::Buffer
{
    return *lightBuffer;
}

void trc::LightRegistry::updateLightBuffer()
{
    assert(lights.size() <= MAX_LIGHTS);

    auto buf = lightBuffer.map();

    const ui32 numLights = lights.size();
    memcpy(buf, &numLights, sizeof(ui32));
    for (size_t offset = sizeof(vec4); const Light* light : lights)
    {
        memcpy(buf + offset, light, sizeof(Light));
        offset += util::sizeof_pad_16_v<Light>;
    }

    lightBuffer.unmap();
}



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
