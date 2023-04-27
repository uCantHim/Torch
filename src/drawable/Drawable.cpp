#include "trc/drawable/Drawable.h"

#include "trc/TorchRenderStages.h"
#include "trc/GBufferPass.h"
#include "trc/RenderPassShadow.h"
#include "trc/drawable/DefaultDrawable.h"



namespace trc
{

Drawable::Drawable(
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

auto Drawable::getGeometry() const -> GeometryID
{
    return geo;
}

auto Drawable::getMaterial() const -> MaterialID
{
    return mat;
}

bool Drawable::isAnimated() const
{
    return scene->hasAnimation(id);
}

auto Drawable::getAnimationEngine() -> AnimationEngine&
{
    return scene->getAnimationEngine(id);
}

auto Drawable::getAnimationEngine() const -> const AnimationEngine&
{
    return scene->getAnimationEngine(id);
}

} // namespace trc
