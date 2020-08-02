#include "Drawable.h"

#include "AssetRegistry.h"
#include "Geometry.h"
#include "Material.h"



trc::Drawable::Drawable(Geometry& geo, ui32 material, SceneBase& scene)
    :
    geo(&geo),
    matIndex(material)
{
    attachToScene(scene);
}

void trc::Drawable::setGeometry(Geometry& geo)
{
    this->geo = &geo;
    currentScene->unregisterDrawFunction(registration);
    updateDrawFunction();
}

void trc::Drawable::setMaterial(ui32 matIndex)
{
    this->matIndex = matIndex;
    currentScene->unregisterDrawFunction(registration);
    updateDrawFunction();
}

void trc::Drawable::makePickable()
{
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

    if (geo->hasRig())
    {
        animEngine = { *geo->getRig() };
        registration = currentScene->registerDrawFunction(
            RenderPasses::eDeferredPass,
            DeferredSubPasses::eGBufferPass,
            Pipelines::eDrawableDeferredAnimated,
            [this](vk::CommandBuffer cmdBuf) {
                drawAnimated(cmdBuf);
            }
        );
    }
    else
    {
        registration = currentScene->registerDrawFunction(
            RenderPasses::eDeferredPass,
            DeferredSubPasses::eGBufferPass,
            Pipelines::eDrawableDeferred,
            [this](vk::CommandBuffer cmdBuf) {
                draw(cmdBuf);
            }
        );
    }
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

void trc::Drawable::drawPickable(vk::CommandBuffer)
{
}

void trc::Drawable::drawAnimatedAndPickable(vk::CommandBuffer)
{
}
