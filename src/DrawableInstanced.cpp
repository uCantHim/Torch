#include "DrawableInstanced.h"

#include "TorchResources.h"



trc::DrawableInstanced::DrawableInstanced(ui32 maxInstances, Geometry& geo)
    :
    geometry(&geo),
    maxInstances(maxInstances),
    instanceDataBuffer(
        sizeof(InstanceDescription) * maxInstances,
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    )
{
}

trc::DrawableInstanced::DrawableInstanced(ui32 maxInstances, Geometry& geo, SceneBase& scene)
    :
    DrawableInstanced(maxInstances, geo)
{
    attachToScene(scene);
}

void trc::DrawableInstanced::attachToScene(SceneBase& scene)
{
    removeFromScene();

    this->scene = &scene;
    scene.registerDrawFunction(
        RenderStageTypes::getDeferred(),
        internal::DeferredSubPasses::eGBufferPass,
        internal::getDrawableInstancedDeferredPipeline(),
        [this](const DrawEnvironment&, vk::CommandBuffer cmdBuf) {
            cmdBuf.bindIndexBuffer(geometry->getIndexBuffer(), 0, vk::IndexType::eUint32);
            cmdBuf.bindVertexBuffers(
                0,
                { geometry->getVertexBuffer(), *instanceDataBuffer },
                { 0ul, 0ul }
            );

            cmdBuf.drawIndexed(geometry->getIndexCount(), numInstances, 0, 0, 0);
        }
    );

    scene.registerDrawFunction(
        RenderStageTypes::getShadow(),
        0,
        internal::getDrawableInstancedShadowPipeline(),
        [this](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
            assert(dynamic_cast<RenderPassShadow*>(env.currentRenderPass) != nullptr);

            uvec2 res = static_cast<RenderPassShadow*>(env.currentRenderPass)->getResolution();
            cmdBuf.setViewport(0, vk::Viewport(0.0f, 0.0f, res.x, res.y, 0.0f, 1.0f));
            cmdBuf.setScissor(0, vk::Rect2D({ 0, 0 }, { res.x, res.y }));

            cmdBuf.bindIndexBuffer(geometry->getIndexBuffer(), 0, vk::IndexType::eUint32);
            cmdBuf.bindVertexBuffers(
                0,
                { geometry->getVertexBuffer(), *instanceDataBuffer },
                { 0ul, 0ul }
            );
            cmdBuf.pushConstants<ui32>(
                env.currentPipeline->getLayout(), vk::ShaderStageFlagBits::eVertex,
                0, static_cast<RenderPassShadow*>(env.currentRenderPass)->getShadowMatrixIndex()
            );

            cmdBuf.drawIndexed(geometry->getIndexCount(), numInstances, 0, 0, 0);
        }
    );
}

void trc::DrawableInstanced::removeFromScene()
{
    if (scene != nullptr)
    {
        scene->unregisterDrawFunction(deferredRegistration);
        scene->unregisterDrawFunction(shadowRegistration);
        scene = nullptr;
    }
}

void trc::DrawableInstanced::setGeometry(Geometry& geo)
{
    geometry = &geo;
}

void trc::DrawableInstanced::addInstance(InstanceDescription instance)
{
    assert(numInstances < maxInstances);

    auto buf = instanceDataBuffer.map(numInstances * sizeof(InstanceDescription));
    memcpy(buf, &instance, sizeof(InstanceDescription));
    instanceDataBuffer.unmap();
    numInstances++;
}
