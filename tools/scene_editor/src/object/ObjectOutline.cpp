#include "ObjectOutline.h"

#include <trc/core/PipelineLayoutBuilder.h>
#include <trc/core/PipelineBuilder.h>
#include <trc/TorchResources.h>

#include "Scene.h"
#include "DefaultAssets.h"



auto getMouseHoverPipeline() -> trc::Pipeline::ID
{
    static auto baseID = trc::getPipeline({});
    static auto layout = trc::PipelineRegistry<trc::TorchRenderConfig>::getPipelineLayout(baseID);

    static auto hoverPipeline = trc::buildGraphicsPipeline(
            trc::PipelineRegistry<trc::TorchRenderConfig>::cloneGraphicsPipeline(baseID)
        )
        .setRasterization(
            vk::PipelineRasterizationStateCreateInfo(trc::DEFAULT_RASTERIZATION)
            .setDepthBiasEnable(true)
            .setDepthBiasConstantFactor(-3.0f)
            .setDepthBiasSlopeFactor(-3.0f)
        )
        .setCullMode(vk::CullModeFlagBits::eFront)
        .disableDepthWrite()
        .registerPipeline<trc::TorchRenderConfig>(
            layout,
            trc::RenderPassName{ trc::TorchRenderConfig::OPAQUE_G_BUFFER_PASS }
        );

    return hoverPipeline;
}



ObjectOutline::ObjectOutline(Scene& scene, SceneObject obj, Type outlineType)
    :
    trc::Drawable(
        { scene.get<trc::Drawable>(obj).getGeometry(), toMaterial(outlineType), false, false },
        getMouseHoverPipeline(),
        scene.getDrawableScene()
    )
{
    this->setScale(OUTLINE_SCALE);
    scene.get<trc::Drawable>(obj).attach(*this);
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
