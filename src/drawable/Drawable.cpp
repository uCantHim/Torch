#include "drawable/Drawable.h"

#include "AssetRegistry.h"
#include "Geometry.h"
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
    Drawable(
        info,
        [&info]{
            PipelineFeatureFlags flags;
            if (info.transparent) {
                flags |= PipelineFeatureFlagBits::eTransparent;
            }
            if (info.geo.get().hasRig()) {
                flags |= PipelineFeatureFlagBits::eAnimated;
            }

            return getPipeline(flags);
        }(),
        scene
    )
{
}

Drawable::Drawable(
    const DrawableCreateInfo& info,
    Pipeline::ID pipeline,
    DrawableComponentScene& scene)
    :
    scene(&scene),
    id(scene.makeDrawable()),
    geo(info.geo)
{
    auto _geo = info.geo.get();

    auto raster = makeRasterData(info, pipeline);
    raster.drawData.geo = _geo;
    raster.drawData.mat = info.mat;
    raster.drawData.modelMatrixId = getGlobalTransformID();
    if (_geo.hasRig())
    {
        scene.makeAnimation(id, *_geo.getRig());
        raster.drawData.anim = scene.getAnimationEngine(id).getState();
    }

    scene.makeRasterization(id, raster);
}

Drawable::Drawable(Drawable&& other) noexcept
    :
    Node(std::forward<Node>(other)),
    scene(other.scene),
    id(other.id),
    geo(other.geo)
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

    return *this;
}

auto Drawable::getMaterial() const -> MaterialID
{
    return scene->getRasterization(id).mat;
}

auto Drawable::getGeometry() const -> GeometryID
{
    return geo;
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
    if (id != DrawableID::NONE) {
        scene->destroyDrawable(id);
    }
}

void Drawable::drawShadow(
    const drawcomp::RasterComponent& data,
    const DrawEnvironment& env,
    vk::CommandBuffer cmdBuf)
{
    auto currentRenderPass = dynamic_cast<RenderPassShadow*>(env.currentRenderPass);
    assert(currentRenderPass != nullptr);

    // Bind buffers and push constants
    data.geo.bindVertices(cmdBuf, 0);

    auto layout = *env.currentPipeline->getLayout();
    cmdBuf.pushConstants<mat4>(
        layout, vk::ShaderStageFlagBits::eVertex,
        0, data.modelMatrixId.get()
    );
    cmdBuf.pushConstants<ui32>(
        layout, vk::ShaderStageFlagBits::eVertex,
        sizeof(mat4), currentRenderPass->getShadowMatrixIndex()
    );
    cmdBuf.pushConstants<AnimationDeviceData>(
        layout, vk::ShaderStageFlagBits::eVertex, sizeof(mat4) + sizeof(ui32),
        data.anim != AnimationEngine::ID::NONE ? data.anim.get() : AnimationDeviceData{}
    );

    // Draw
    cmdBuf.drawIndexed(data.geo.getIndexCount(), 1, 0, 0, 0);
}

auto Drawable::makeRasterData(
    const DrawableCreateInfo& info,
    Pipeline::ID gBufferPipeline
    ) -> RasterComponentCreateInfo
{
    return makeDefaultDrawableRasterization(info, gBufferPipeline);
}

} // namespace trc
