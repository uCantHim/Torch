#include "Light.h"

#include "utils/Util.h"



trc::LightNode::LightNode(Light& light)
    :
    light(&light),
    initialDirection(light.direction)
{
}

void trc::LightNode::update()
{
    assert(light != nullptr);

    light->position = { getTranslation(), 1.0f };
    light->direction = getRotationAsMatrix() * initialDirection;
}



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
    for (auto& [light, node] : lightNodes)
    {
        node->update();
    }

    updateLightBuffer();
}

auto trc::LightRegistry::addLight(const Light& light) -> const Light&
{
    return *lights.emplace_back(&light);
}

void trc::LightRegistry::removeLight(const Light& light)
{
    removeLightNode(light);
    auto it = std::remove(lights.begin(), lights.end(), &light);
    if (it != lights.end()) {
        lights.erase(it);
    }
}

auto trc::LightRegistry::createLightNode(Light& light) -> LightNode&
{
    return *lightNodes.emplace_back(
        &light,
        std::make_unique<LightNode>(light)
    ).second;
}

void trc::LightRegistry::removeLightNode(const LightNode& node)
{
    auto it = std::find_if(
        lightNodes.begin(), lightNodes.end(),
        [&node](const auto& pair) { return pair.second.get() == &node; }
    );

    if (it != lightNodes.end()) {
        lightNodes.erase(it);
    }
}

void trc::LightRegistry::removeLightNode(const Light& light)
{
    auto it = std::find_if(
        lightNodes.begin(), lightNodes.end(),
        [&light](const auto& pair) { return pair.first == &light; }
    );

    if (it != lightNodes.end()) {
        lightNodes.erase(it);
    }
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
