#include "Drawable.h"

#include "AssetRegistry.h"
#include "Geometry.h"
#include "Material.h"



trc::Drawable::Drawable(Geometry& geo, ui32 material, SceneBase& scene)
{
    setGeometry(geo);
    setMaterial(material);

    attachToScene(scene);
}

trc::Drawable::~Drawable()
{
    //PickableRegistry::destroyPickable(PickableRegistry::getPickable(pickableId));
}

void trc::Drawable::setGeometry(Geometry& geo)
{
    this->geo = &geo;
    if (geo.hasRig()) {
        animEngine = { *geo.getRig() };
    }

    if (currentScene != nullptr) {
        currentScene->unregisterDrawFunction(registration);
    }
    updateDrawFunction();
}

void trc::Drawable::setMaterial(ui32 matIndex)
{
    this->matIndex = matIndex;

    if (currentScene != nullptr) {
        currentScene->unregisterDrawFunction(registration);
    }
    updateDrawFunction();
}

auto trc::Drawable::getAnimationEngine() noexcept -> AnimationEngine&
{
    return animEngine;
}

void trc::Drawable::attachToScene(SceneBase& scene)
{
    if (currentScene != nullptr) {
        currentScene->unregisterDrawFunction(registration);
    }

    currentScene = &scene;
    updateDrawFunction();
}

void trc::Drawable::updateDrawFunction()
{
    if (currentScene == nullptr || geo == nullptr) {
        return;
    }

    DrawableFunction func;
    GraphicsPipeline::ID pipeline;
    if (geo->hasRig())
    {

        if (pickableId == NO_PICKABLE) {
            func = [this](vk::CommandBuffer cmdBuf) { drawAnimated(cmdBuf); };
            pipeline = Pipelines::eDrawableDeferredAnimated;
        }
        else {
            func = [this](vk::CommandBuffer cmdBuf) { drawAnimatedAndPickable(cmdBuf); };
            pipeline = Pipelines::eDrawableDeferredAnimatedAndPickable;
        }
    }
    else
    {
        if (pickableId == NO_PICKABLE) {
            func = [this](vk::CommandBuffer cmdBuf) { draw(cmdBuf); };
            pipeline = Pipelines::eDrawableDeferred;
        }
        else {
            func = [this](vk::CommandBuffer cmdBuf) { drawPickable(cmdBuf); };
            pipeline = Pipelines::eDrawableDeferredPickable;
        }
    }

    registration = currentScene->registerDrawFunction(
        RenderPasses::eDeferredPass,
        DeferredSubPasses::eGBufferPass,
        pipeline,
        std::move(func)
    );
}

void trc::Drawable::prepareDraw(vk::CommandBuffer cmdBuf, vk::PipelineLayout layout)
{
    cmdBuf.bindIndexBuffer(geo->getIndexBuffer(), 0, vk::IndexType::eUint32);
    cmdBuf.bindVertexBuffers(0, geo->getVertexBuffer(), vk::DeviceSize(0));

    cmdBuf.pushConstants<mat4>(
        layout, vk::ShaderStageFlagBits::eVertex,
        0, getGlobalTransform()
    );
    cmdBuf.pushConstants<ui32>(
        layout, vk::ShaderStageFlagBits::eVertex,
        sizeof(mat4), matIndex
    );
}

void trc::Drawable::draw(vk::CommandBuffer cmdBuf)
{
    auto layout = GraphicsPipeline::at(Pipelines::eDrawableDeferred).getLayout();
    prepareDraw(cmdBuf, layout);

    cmdBuf.drawIndexed(geo->getIndexCount(), 1, 0, 0, 0);
}

void trc::Drawable::drawAnimated(vk::CommandBuffer cmdBuf)
{
    auto layout = GraphicsPipeline::at(Pipelines::eDrawableDeferredAnimated).getLayout();
    prepareDraw(cmdBuf, layout);
    animEngine.pushConstants(sizeof(mat4) + sizeof(ui32), layout, cmdBuf);

    cmdBuf.drawIndexed(geo->getIndexCount(), 1, 0, 0, 0);
}

void trc::Drawable::drawPickable(vk::CommandBuffer cmdBuf)
{
    auto layout = GraphicsPipeline::at(Pipelines::eDrawableDeferredPickable).getLayout();
    prepareDraw(cmdBuf, layout);
    cmdBuf.pushConstants<ui32>(layout, vk::ShaderStageFlagBits::eFragment, 84, pickableId);

    cmdBuf.drawIndexed(geo->getIndexCount(), 1, 0, 0, 0);
}

void trc::Drawable::drawAnimatedAndPickable(vk::CommandBuffer cmdBuf)
{
    auto layout = GraphicsPipeline::at(Pipelines::eDrawableDeferredAnimatedAndPickable).getLayout();
    prepareDraw(cmdBuf, layout);
    animEngine.pushConstants(sizeof(mat4) + sizeof(ui32), layout, cmdBuf);
    cmdBuf.pushConstants<ui32>(layout, vk::ShaderStageFlagBits::eFragment, 84, pickableId);

    cmdBuf.drawIndexed(geo->getIndexCount(), 1, 0, 0, 0);
}
