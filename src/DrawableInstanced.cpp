#include "DrawableInstanced.h"



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

void trc::DrawableInstanced::recordCommandBuffer(
    Deferred,
    const DrawEnvironment&,
    vk::CommandBuffer cmdBuf)
{
    cmdBuf.bindIndexBuffer(geometry->getIndexBuffer(), 0, vk::IndexType::eUint32);
    cmdBuf.bindVertexBuffers(0, { geometry->getVertexBuffer(), *instanceDataBuffer }, { 0ul, 0ul });

    cmdBuf.drawIndexed(geometry->getIndexCount(), numInstances, 0, 0, 0);
}

void trc::DrawableInstanced::recordCommandBuffer(
    Shadow,
    const DrawEnvironment& env,
    vk::CommandBuffer cmdBuf)
{
    assert(dynamic_cast<RenderPassShadow*>(env.currentRenderPass) != nullptr);

    cmdBuf.bindIndexBuffer(geometry->getIndexBuffer(), 0, vk::IndexType::eUint32);
    cmdBuf.bindVertexBuffers(0, { geometry->getVertexBuffer(), *instanceDataBuffer }, { 0ul, 0ul });
    cmdBuf.pushConstants<ui32>(
        env.currentPipeline->getLayout(), vk::ShaderStageFlagBits::eVertex,
        0, static_cast<RenderPassShadow*>(env.currentRenderPass)->getShadowIndex()
    );

    cmdBuf.drawIndexed(geometry->getIndexCount(), numInstances, 0, 0, 0);
}
