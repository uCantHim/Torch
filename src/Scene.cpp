#include "Scene.h"

#include <cstring>

#include "utils/Util.h"
#include "PickableRegistry.h"



trc::Scene::Scene()
    :
    pickingBuffer(
        sizeof(ui32) * 3,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    descriptor(*this)
{
    auto buf = reinterpret_cast<ui32*>(pickingBuffer.map());
    buf[0] = 0u;
    buf[1] = 0u;
    reinterpret_cast<float*>(buf)[2] = 1.0f;
    pickingBuffer.unmap();
}

auto trc::Scene::getRoot() noexcept -> Node&
{
    return root;
}

auto trc::Scene::getRoot() const noexcept -> const Node&
{
    return root;
}

void trc::Scene::update()
{
    lightRegistry.update();
    updatePicking();
}

void trc::Scene::updateTransforms()
{
    root.updateAsRoot();
}

void trc::Scene::addLight(const Light& light)
{
    lightRegistry.addLight(light);
}

void trc::Scene::removeLight(const Light& light)
{
    lightRegistry.removeLight(light);
}

auto trc::Scene::getLightBuffer() const noexcept -> vk::Buffer
{
    return lightRegistry.getLightBuffer();
}

auto trc::Scene::getLightRegistry() noexcept -> LightRegistry&
{
    return lightRegistry;
}

auto trc::Scene::getLightRegistry() const noexcept -> const LightRegistry&
{
    return lightRegistry;
}

auto trc::Scene::getDescriptor() const noexcept -> const SceneDescriptor&
{
    return descriptor;
}

auto trc::Scene::getPickingBuffer() const noexcept -> vk::Buffer
{
    return *pickingBuffer;
}

auto trc::Scene::getPickedObject() -> std::optional<Pickable*>
{
    if (currentlyPicked == 0) {
        return std::nullopt;
    }

    return &PickableRegistry::getPickable(currentlyPicked);
}

void trc::Scene::updatePicking()
{
    auto buf = reinterpret_cast<ui32*>(pickingBuffer.map());
    ui32 newPicked = buf[0];
    buf[0] = 0u;
    buf[1] = 0u;
    reinterpret_cast<float*>(buf)[2] = 1.0f;
    pickingBuffer.unmap();

    // Unpick previously picked object
    if (newPicked != currentlyPicked)
    {
        if (currentlyPicked != 0)
        {
            PickableRegistry::getPickable(currentlyPicked).onUnpick();
        }
        if (newPicked != 0)
        {
            PickableRegistry::getPickable(newPicked).onPick();
        }
        currentlyPicked = newPicked;
    }
}
