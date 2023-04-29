#include "trc/drawable/Drawable.h"

#include "trc/drawable/AnimationComponent.h"



namespace trc
{

DrawableObj::DrawableObj(
    DrawableID id,
    DrawableComponentScene& scene,
    GeometryID geometry,
    MaterialID material)
    :
    scene(&scene),
    id(id),
    geo(geometry),
    mat(material)
{
}

auto DrawableObj::getGeometry() const -> GeometryID
{
    return geo;
}

auto DrawableObj::getMaterial() const -> MaterialID
{
    return mat;
}

bool DrawableObj::isAnimated() const
{
    return scene->has<AnimationComponent>(id);
}

auto DrawableObj::getAnimationEngine() -> std::optional<AnimationEngine*>
{
    if (auto comp = scene->tryGet<AnimationComponent>(id)) {
        return &comp.value()->engine;
    }
    return std::nullopt;
}

} // namespace trc
