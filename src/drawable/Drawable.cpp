#include "drawable/Drawable.h"

#include "GeometryRegistry.h"
#include "Material.h"
#include "TorchResources.h"
#include "GBufferPass.h"
#include "RenderPassShadow.h"
#include "drawable/DefaultDrawable.h"



namespace trc
{

Drawable::Drawable(GeometryID geo, MaterialID material, DrawableComponentScene& scene)
    :
    Drawable({ geo, material }, scene)
{
}

Drawable::Drawable(const DrawableCreateInfo& info, DrawableComponentScene& scene)
    :
    Drawable(info, determineDrawablePipeline(info), scene)
{
}

Drawable::Drawable(
    const DrawableCreateInfo& info,
    Pipeline::ID pipeline,
    DrawableComponentScene& scene)
    :
    scene(&scene),
    id(scene.makeDrawable())
{
    auto geo = info.geo.get();

    auto raster = makeRasterData(info, pipeline);

    // Model matrix ID and animation engine ID have to be set manually
    raster.drawData.modelMatrixId = getGlobalTransformID();
    if (geo.hasRig())
    {
        scene.makeAnimationEngine(id, geo.getRig());
        raster.drawData.anim = scene.getAnimationEngine(id).getState();
    }

    scene.makeRasterization(id, raster);
}

Drawable::Drawable(Drawable&& other) noexcept
    :
    Node(std::forward<Node>(other)),
    scene(other.scene),
    id(other.id)
{
    other.scene = nullptr;
    other.id = DrawableID::NONE;
}

Drawable::~Drawable()
{
    if (id != DrawableID::NONE && scene != nullptr)
    {
        scene->destroyDrawable(id);
    }
}

auto Drawable::operator=(Drawable&& other) noexcept -> Drawable&
{
    Node::operator=(std::forward<Node>(other));

    std::swap(scene, other.scene);
    std::swap(id, other.id);

    return *this;
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

void Drawable::removeFromScene()
{
    if (id != DrawableID::NONE)
    {
        scene->destroyDrawable(id);
        id = DrawableID::NONE;
    }
}

auto Drawable::makeRasterData(
    const DrawableCreateInfo& info,
    Pipeline::ID gBufferPipeline
    ) -> RasterComponentCreateInfo
{
    return makeDefaultDrawableRasterization(info, gBufferPipeline);
}

} // namespace trc
