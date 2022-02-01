#include "ObjectSelection.h"

#include <trc/core/PipelineLayoutBuilder.h>
#include <trc/core/PipelineBuilder.h>
#include <trc/TorchResources.h>

#include "Scene.h"
#include "AssetManager.h"
#include "DefaultAssets.h"



auto getMouseHoverPipeline() -> trc::Pipeline::ID
{
    static auto baseID = trc::getPipeline({});
    static auto layout = trc::PipelineRegistry<trc::TorchRenderConfig>::getPipelineLayout(baseID);

    static auto hoverPipeline = trc::GraphicsPipelineBuilder(
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



ObjectSelection::ObjectSelection(Scene& scene)
    :
    scene(&scene)
{
}

void ObjectSelection::hoverObject(SceneObject obj)
{
    if (hoveredObject != obj)
    {
        unhoverObject();
        hoveredObject = obj;

        if (hoveredObject != SceneObject::NONE)
        {
            trc::Drawable& d = scene->get<trc::Drawable>(obj);
            objectShadeDrawable = trc::Drawable(
                { d.getGeometry(), g::mats().objectHighlight, false, false },
                getMouseHoverPipeline()
            );
            objectShadeDrawable.setScale(1.02f);
            objectShadeDrawable.attachToScene(scene->getDrawableScene());
            d.attach(objectShadeDrawable);
        }
    }
}

void ObjectSelection::unhoverObject()
{
    hoveredObject = SceneObject::NONE;
    objectShadeDrawable = {};
}

auto ObjectSelection::getHoveredObject() const -> SceneObject
{
    return hoveredObject;
}

bool ObjectSelection::hasHoveredObject() const
{
    return getHoveredObject() != SceneObject::NONE;
}

void ObjectSelection::selectObject(SceneObject obj)
{
    unselectObject();
    selectedObject = obj;
}

void ObjectSelection::unselectObject()
{
    selectedObject = SceneObject::NONE;
}

auto ObjectSelection::getSelectedObject() const -> SceneObject
{
    return selectedObject;
}

bool ObjectSelection::hasSelectedObject() const
{
    return getSelectedObject() != SceneObject::NONE;
}
