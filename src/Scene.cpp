#include "Scene.h"

#include <cstring>

#include "utils/Util.h"



trc::Scene::Scene()
    :
    lightBuffer(
        util::sizeof_pad_16_v<Light> * MAX_LIGHTS,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    )
{
}

auto trc::Scene::getRoot() noexcept -> Node&
{
    return root;
}

auto trc::Scene::getRoot() const noexcept -> const Node&
{
    return root;
}

void trc::Scene::updateTransforms()
{
    root.updateAsRoot();
    updateLightBuffer();
}

void trc::Scene::add(SceneRegisterable& object)
{
    object.attachToScene(*this);
}

void trc::Scene::addLight(Light& light)
{
    lights.push_back(&light);
}

void trc::Scene::removeLight(Light& light)
{
    lights.erase(std::find(lights.begin(), lights.end(), &light));
}

auto trc::Scene::getLightBuffer() const noexcept -> vk::Buffer
{
    return *lightBuffer;
}

void trc::Scene::updateLightBuffer()
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
