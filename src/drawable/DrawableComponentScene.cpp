#include "trc/drawable/DrawableComponentScene.h"

#include "trc/drawable/AnimationComponent.h"
#include "trc/drawable/RayComponent.h"



namespace trc
{

UniqueDrawableID::UniqueDrawableID(DrawableID drawable, DrawableComponentScene& scene)
    :
    id(drawable),
    scene(&scene)
{
}

UniqueDrawableID::UniqueDrawableID(UniqueDrawableID&& other) noexcept
    :
    id(other.id),
    scene(other.scene)
{
    other.id = DrawableID::NONE;
    other.scene = nullptr;
}

UniqueDrawableID& UniqueDrawableID::operator=(UniqueDrawableID&& other) noexcept
{
    std::swap(id, other.id);
    std::swap(scene, other.scene);
    return *this;
}

UniqueDrawableID::~UniqueDrawableID() noexcept
{
    if (id != DrawableID::NONE && scene != nullptr)
    {
        scene->destroyDrawable(id);
    }
}



DrawableComponentScene::DrawableComponentScene()
{
    registerModule(std::make_unique<RasterSceneModule>());
    registerModule(std::make_unique<RaySceneModule>());
}

auto DrawableComponentScene::getRasterModule() -> RasterSceneModule&
{
    return getModule<RasterSceneModule>();
}

auto DrawableComponentScene::getRayModule() -> RaySceneModule&
{
    return getModule<RaySceneModule>();
}

void DrawableComponentScene::updateAnimations(const float timeDelta)
{
    for (auto& anim : get<AnimationComponent>())
    {
        anim.engine.update(timeDelta);
    }
}

void DrawableComponentScene::updateRayInstances()
{
    for (const auto& ray : get<RayComponent>()) {
        getRayModule().setInstanceTransform(ray.instanceDataIndex, ray.modelMatrix.get());
    }
}

} // namespace trc
