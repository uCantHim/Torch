#include "ObjectOutline.h"

#include <trc/Torch.h>
#include <trc/core/PipelineLayoutBuilder.h>
#include <trc/core/PipelineBuilder.h>
#include <trc/drawable/DefaultDrawable.h>
#include <trc/DrawablePipelines.h>

#include "asset/DefaultAssets.h"
#include "Scene.h"

auto getObjectOutlinePipeline() -> trc::Pipeline::ID
{
    static auto baseID = trc::pipelines::getDrawablePipeline(
        trc::pipelines::PipelineShadingTypeFlagBits::opaque
        | trc::pipelines::AnimationTypeFlagBits::none
    );
    static auto layout = trc::PipelineRegistry<trc::TorchRenderConfig>::getPipelineLayout(baseID);

    static auto hoverPipeline = trc::buildGraphicsPipeline(
            trc::PipelineRegistry<trc::TorchRenderConfig>::cloneGraphicsPipeline(baseID)
        )
        .setRasterization(
            vk::PipelineRasterizationStateCreateInfo(trc::DEFAULT_RASTERIZATION)
            .setDepthBiasEnable(true)
            .setDepthBiasConstantFactor(3.0f)
            .setDepthBiasSlopeFactor(3.0f)
        )
        .setCullMode(vk::CullModeFlagBits::eFront)
        .disableDepthWrite()
        .registerPipeline<trc::TorchRenderConfig>(
            layout,
            trc::RenderPassName{ trc::TorchRenderConfig::OPAQUE_G_BUFFER_PASS }
        );

    return hoverPipeline;
}

auto getAnimatedObjectOutlinePipeline() -> trc::Pipeline::ID
{
    static auto baseID = trc::pipelines::getDrawablePipeline(
        trc::pipelines::PipelineShadingTypeFlagBits::opaque
        | trc::pipelines::AnimationTypeFlagBits::boneAnim
    );
    static auto layout = trc::PipelineRegistry<trc::TorchRenderConfig>::getPipelineLayout(baseID);

    static auto hoverPipeline = trc::buildGraphicsPipeline(
            trc::PipelineRegistry<trc::TorchRenderConfig>::cloneGraphicsPipeline(baseID)
        )
        .setRasterization(
            vk::PipelineRasterizationStateCreateInfo(trc::DEFAULT_RASTERIZATION)
            .setDepthBiasEnable(true)
            .setDepthBiasConstantFactor(3.0f)
            .setDepthBiasSlopeFactor(3.0f)
        )
        .setCullMode(vk::CullModeFlagBits::eFront)
        .disableDepthWrite()
        .registerPipeline<trc::TorchRenderConfig>(
            layout,
            trc::RenderPassName{ trc::TorchRenderConfig::OPAQUE_G_BUFFER_PASS }
        );

    return hoverPipeline;
}



ObjectOutline::ObjectOutline(Scene& _scene, SceneObject obj, Type outlineType)
    :
    drawable(_scene.getDrawableScene().makeDrawableUnique())
{
    auto& scene = _scene.getDrawableScene();
    auto& d = _scene.get<trc::Drawable>(obj);

    auto geo = d.getGeometry();
    auto mat = toMaterial(outlineType);

    scene.makeRasterization(drawable, trc::RasterComponentCreateInfo{
            .drawData={
                .geo           = geo.getDeviceDataHandle(),
                .mat           = mat.getDeviceDataHandle(),
                .modelMatrixId = node->getGlobalTransformID(),
                .anim          = d.isAnimated()
                                 ? d.getAnimationEngine().getState()
                                 : trc::AnimationEngine::ID{}
            },
            .drawFunctions=trc::makeDefaultDrawableRasterization(
                { geo, mat, false, false },
                d.isAnimated() ? getAnimatedObjectOutlinePipeline() : getObjectOutlinePipeline()
            ).drawFunctions
        }
    );

    node->setScale(OUTLINE_SCALE);
    d.attach(*node);
}

auto ObjectOutline::toMaterial(Type type) -> trc::MaterialID
{
    switch (type)
    {
        case Type::eHover:  return g::mats().objectHighlight;
        case Type::eSelect: return g::mats().objectSelect;
    }

    throw std::logic_error("Invalid ObjectOutline type");
}



ObjectHoverOutline::ObjectHoverOutline(Scene& scene, SceneObject obj)
    : ObjectOutline(scene, obj, Type::eHover)
{
}

ObjectSelectOutline::ObjectSelectOutline(Scene& scene, SceneObject obj)
    : ObjectOutline(scene, obj, Type::eSelect)
{
}
