#include "LightRegistry.h"

#include <vkb/ImageUtils.h>

#include "core/Instance.h"
#include "utils/Util.h"
#include "PipelineDefinitions.h"



auto trc::LightRegistry::ShadowInfo::getNode() noexcept -> Node&
{
    return parentNode;
}

void trc::LightRegistry::ShadowInfo::setProjectionMatrix(mat4 proj) noexcept
{
    for (auto& camera : shadowCameras) {
        camera.setProjectionMatrix(proj);
    }
}



//////////////////////////////
//      Light registry      //
//////////////////////////////

trc::LightRegistry::LightRegistry(const Instance& instance, const ui32 maxLights)
    :
    maxLights(maxLights),
    maxShadowMaps(glm::min(maxLights * 4, MAX_SHADOW_MAPS)),
    lightBuffer(
        instance.getDevice(),
        util::sizeof_pad_16_v<Light> * maxLights,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    shadowMatrixBuffer(
        instance.getDevice(),
        sizeof(mat4) * maxShadowMaps,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    )
{
}

void trc::LightRegistry::update()
{
    // TODO: Put these into a single buffer
    updateLightBuffer();
    updateShadowMatrixBuffer();
}

auto trc::LightRegistry::getMaxLights() const noexcept -> ui32
{
    return maxLights;
}

auto trc::LightRegistry::addLight(Light light) -> Light&
{
    switch (light.type)
    {
    case Light::Type::eSunLight:
        return *sunLights.emplace_back(new Light(light));
    case Light::Type::ePointLight:
        return *pointLights.emplace_back(new Light(light));
    case Light::Type::eAmbientLight:
        return *ambientLights.emplace_back(new Light(light));
    }

    throw std::logic_error("Light type \"" + std::to_string(light.type) + "\" exists");
}

void trc::LightRegistry::removeLight(const Light& light)
{
    auto remove = [](std::vector<u_ptr<Light>>& lights, const Light& light) {
        auto it = std::remove_if(lights.begin(), lights.end(),
                                 [&](auto& l) { return l.get() == &light; });
        if (it != lights.end()) {
            lights.erase(it);
        }
    };

    switch (light.type)
    {
    case Light::Type::eSunLight:
        remove(sunLights, light);
        break;
    case Light::Type::ePointLight:
        remove(pointLights, light);
        break;
    case Light::Type::eAmbientLight:
        remove(ambientLights, light);
        break;
    }
}

auto trc::LightRegistry::enableShadow(
    Light& light,
    uvec2 shadowResolution
    ) -> ShadowInfo&
{
    if (light.type != Light::Type::eSunLight) {
        throw std::invalid_argument("Shadows are currently only supported for sun lights");
    }
    if (lightExists(light)) {
        throw std::invalid_argument("Light does not exist in the light registry!");
    }
    if (shadows.find(&light) != shadows.end()) {
        throw std::invalid_argument("Shadows are already enabled for the light!");
    }

    auto [it, success] = shadows.try_emplace(&light);
    if (!success) {
        throw std::runtime_error("Unable to add light to the map in LightRegistry::enableShadow");
    }

    auto& newEntry = it->second;
    for (ui32 i = 0; i < getNumShadowMaps(light); i++)
    {
        auto& camera = newEntry.shadowCameras.emplace_back();
        newEntry.parentNode.attach(camera);
        // Use lookAt for sun lights
        if (light.type == Light::Type::eSunLight && length(light.direction) > 0.0f) {
            camera.lookAt(light.position, light.position + light.direction, vec3(0, 1, 0));
        }
    }

    newEntry.parentNode.update();
    light.hasShadow = true;

    return newEntry;
}

void trc::LightRegistry::disableShadow(Light& light)
{
    if (!light.hasShadow) return;

    auto it = shadows.find(&light);
    if (it != shadows.end()) {
        shadows.erase(it);
    }
}

auto trc::LightRegistry::getLightBuffer() const noexcept -> vk::Buffer
{
    return *lightBuffer;
}

auto trc::LightRegistry::getShadowMatrixBuffer() const noexcept -> vk::Buffer
{
    return *shadowMatrixBuffer;
}

bool trc::LightRegistry::lightExists(const Light& light)
{
    auto compare = [&](auto& ptr) { return &light == ptr.get(); };
    switch (light.type)
    {
    case Light::Type::eSunLight:
        return std::find_if(sunLights.begin(), sunLights.end(), compare) == sunLights.end();
    case Light::Type::ePointLight:
        return std::find_if(pointLights.begin(), pointLights.end(), compare) == pointLights.end();
    case Light::Type::eAmbientLight:
        return std::find_if(ambientLights.begin(), ambientLights.end(), compare) == ambientLights.end();
    }

    throw std::logic_error("Light type \"" + std::to_string(light.type) + "\" exists");
}

void trc::LightRegistry::updateLightBuffer()
{
    assert(sunLights.size() + pointLights.size() + ambientLights.size() <= maxLights);

    auto buf = lightBuffer.map();

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
        memcpy(buf + offset, &*light, sizeof(Light));
        offset += util::sizeof_pad_16_v<Light>;
    }
    for (const auto& light : pointLights)
    {
        memcpy(buf + offset, &*light, sizeof(Light));
        offset += util::sizeof_pad_16_v<Light>;
    }
    for (const auto& light : ambientLights)
    {
        memcpy(buf + offset, &*light, sizeof(Light));
        offset += util::sizeof_pad_16_v<Light>;
    }

    lightBuffer.unmap();
}

void trc::LightRegistry::updateShadowMatrixBuffer()
{
    if (shadows.empty()) {
        return;
    }

    mat4* buf = reinterpret_cast<mat4*>(shadowMatrixBuffer.map());
    for (size_t i = 0; const auto& [light, shadow] : shadows)
    {
        for (const auto& camera : shadow.shadowCameras)
        {
            // Only increase counter for lights that have a shadow
            buf[i++] = camera.getProjectionMatrix() * camera.getViewMatrix();
        }
    }
    shadowMatrixBuffer.unmap();
}
