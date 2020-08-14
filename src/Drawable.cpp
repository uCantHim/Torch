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
        currentScene->unregisterDrawFunction(deferredRegistration);
        currentScene->unregisterDrawFunction(shadowRegistration);
    }
    updateDrawFunction();
}

void trc::Drawable::setMaterial(ui32 matIndex)
{
    this->matIndex = matIndex;

    if (currentScene != nullptr) {
        currentScene->unregisterDrawFunction(deferredRegistration);
        currentScene->unregisterDrawFunction(shadowRegistration);
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
        currentScene->unregisterDrawFunction(deferredRegistration);
        currentScene->unregisterDrawFunction(shadowRegistration);
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
            func = [this](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
                drawAnimated(env, cmdBuf);
            };
            pipeline = Pipelines::eDrawableDeferredAnimated;
        }
        else {
            func = [this](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
                drawAnimatedAndPickable(env, cmdBuf);
            };
            pipeline = Pipelines::eDrawableDeferredAnimatedAndPickable;
        }
    }
    else
    {
        if (pickableId == NO_PICKABLE) {
            func = [this](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
                draw(env, cmdBuf);
            };
            pipeline = Pipelines::eDrawableDeferred;
        }
        else {
            func = [this](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
                drawPickable(env, cmdBuf);
            };
            pipeline = Pipelines::eDrawableDeferredPickable;
        }
    }

    deferredRegistration = currentScene->registerDrawFunction(
        RenderStages::eDeferred,
        DeferredSubPasses::eGBufferPass,
        pipeline,
        std::move(func)
    );
    shadowRegistration = currentScene->registerDrawFunction(
        RenderStages::eShadow, 0, Pipelines::eDrawableShadow,
        [this](const auto& env, vk::CommandBuffer cmdBuf) { drawShadow(env, cmdBuf); }
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

void trc::Drawable::draw(const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
{
    auto layout = env.currentPipeline->getLayout();
    prepareDraw(cmdBuf, layout);

    cmdBuf.drawIndexed(geo->getIndexCount(), 1, 0, 0, 0);
}

void trc::Drawable::drawAnimated(const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
{
    auto layout = env.currentPipeline->getLayout();
    prepareDraw(cmdBuf, layout);
    animEngine.pushConstants(sizeof(mat4) + sizeof(ui32), layout, cmdBuf);

    cmdBuf.drawIndexed(geo->getIndexCount(), 1, 0, 0, 0);
}

void trc::Drawable::drawPickable(const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
{
    auto layout = env.currentPipeline->getLayout();
    prepareDraw(cmdBuf, layout);
    cmdBuf.pushConstants<ui32>(layout, vk::ShaderStageFlagBits::eFragment, 84, pickableId);

    cmdBuf.drawIndexed(geo->getIndexCount(), 1, 0, 0, 0);
}

void trc::Drawable::drawAnimatedAndPickable(const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
{
    auto layout = env.currentPipeline->getLayout();
    prepareDraw(cmdBuf, layout);
    animEngine.pushConstants(sizeof(mat4) + sizeof(ui32), layout, cmdBuf);
    cmdBuf.pushConstants<ui32>(layout, vk::ShaderStageFlagBits::eFragment, 84, pickableId);

    cmdBuf.drawIndexed(geo->getIndexCount(), 1, 0, 0, 0);
}

void trc::Drawable::drawShadow(const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
{
    ivec2 res = static_cast<SunShadowPass*>(env.currentRenderPass)->getResolution();
    cmdBuf.setViewport(0, vk::Viewport(0.0f, 0.0f, res.x, res.y, 0.0f, 1.0f));
    cmdBuf.setScissor(0, {{ 0, 0 }, { res.x, static_cast<ui32>(res.y) }});

    cmdBuf.bindIndexBuffer(geo->getIndexBuffer(), 0, vk::IndexType::eUint32);
    cmdBuf.bindVertexBuffers(0, geo->getVertexBuffer(), vk::DeviceSize(0));

    auto layout = env.currentPipeline->getLayout();
    cmdBuf.pushConstants<mat4>(
        layout, vk::ShaderStageFlagBits::eVertex,
        0, getGlobalTransform()
    );
    cmdBuf.pushConstants<ui32>(
        layout, vk::ShaderStageFlagBits::eVertex,
        sizeof(mat4), env.currentRenderPass->id()
    );
    cmdBuf.pushConstants<ui32>(
        layout, vk::ShaderStageFlagBits::eVertex,
        sizeof(mat4) + sizeof(ui32), false
    );

    cmdBuf.drawIndexed(geo->getIndexCount(), 1, 0, 0, 0);
}
