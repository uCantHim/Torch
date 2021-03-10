#include "Scene.h"

#include <cstring>

#include "utils/Util.h"
#include "PickableRegistry.h"



trc::Scene::Scene()
    :
    descriptor(*this)
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
    updatePicking();
}

void trc::Scene::updateTransforms()
{
    root.updateAsRoot();
}

void trc::Scene::addLight(Light& light)
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

auto trc::Scene::getLocalRenderPasses(RenderStageType::ID stageType)
    -> std::vector<RenderPass::ID>
{
    if (stageType == RenderStageTypes::getShadow()) {
        return lightRegistry.getShadowRenderStage();
    }

    return {};
}

auto trc::Scene::getDescriptor() const noexcept -> const SceneDescriptor&
{
    return descriptor;
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
    descriptor.updatePicking().maybe(
        // An object is being picked
        [this](ui32 newPicked) {
            if (newPicked != currentlyPicked)
            {
                if (currentlyPicked != NO_PICKABLE)
                {
                    PickableRegistry::getPickable(currentlyPicked).onUnpick();
                    currentlyPicked = NO_PICKABLE;
                }
                PickableRegistry::getPickable(newPicked).onPick();
                currentlyPicked = newPicked;
            }
        },
        // No object is picked
        [this] {
            if (currentlyPicked != NO_PICKABLE)
            {
                PickableRegistry::getPickable(currentlyPicked).onUnpick();
                currentlyPicked = NO_PICKABLE;
            }
        }
    );
}
