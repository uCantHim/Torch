#include "Scene.h"

#include <cstring>

#include "utils/Util.h"
#include "PickableRegistry.h"
#include "TorchResources.h"


trc::Scene::Scene(const Instance& instance)
    :
    lightRegistry(instance)
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

void trc::Scene::update()
{
    lightRegistry.update();
}

void trc::Scene::updateTransforms()
{
    root.updateAsRoot();
}

auto trc::Scene::addLight(Light light) -> Light&
{
    return lightRegistry.addLight(light);
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
