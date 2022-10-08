#include "trc/drawable/Drawable.h"

#include "trc/TorchResources.h"
#include "trc/GBufferPass.h"
#include "trc/RenderPassShadow.h"
#include "trc/drawable/DefaultDrawable.h"



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
    id(scene.makeDrawable()),
    geo(info.geo),
    mat(info.mat)
{
    if (info.rasterized)
    {
        auto geoHandle = info.geo.get();
        auto raster = makeRasterData(info, pipeline);

        // Model matrix ID and animation engine ID have to be set manually
        raster.drawData.modelMatrixId = getGlobalTransformID();
        if (geoHandle.hasRig())
        {
            scene.makeAnimationEngine(id, geoHandle.getRig().getDeviceDataHandle());
            raster.drawData.anim = scene.getAnimationEngine(id).getState();
        }

        scene.makeRasterization(id, raster);
    }

    if (info.rayTraced) {
        scene.makeRaytracing(id, { info.geo, info.mat, getGlobalTransformID() });
    }
}

Drawable::Drawable(Drawable&& other) noexcept
    :
    Node(std::forward<Node>(other)),
    scene(other.scene),
    id(other.id),
    geo(other.geo),
    mat(other.mat)
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
    std::swap(geo, other.geo);
    std::swap(mat, other.mat);

    return *this;
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
