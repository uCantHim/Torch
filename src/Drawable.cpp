#include "Drawable.h"

#include "AssetRegistry.h"
#include "Geometry.h"
#include "Material.h"



trc::Drawable::Drawable(Geometry& geo, ui32 mat)
    :
    indexBuffer(geo.getIndexBuffer()),
    vertexBuffer(geo.getVertexBuffer()),
    indexCount(geo.getIndexCount()),
    material(mat)
{
    if (geo.hasRig())
    {
        assert(geo.getRig() != nullptr);
        animEngine = { *geo.getRig() };
        isAnimated = true;
    }
}

trc::Drawable::Drawable(Geometry& geo, ui32 mat, SceneBase& scene)
    :
    Drawable(geo, mat)
{
    attachToScene(scene);
}

auto trc::Drawable::getMaterial() const noexcept -> ui32
{
    return material;
}

void trc::Drawable::setGeometry(Geometry& geo)
{
    indexBuffer = geo.getIndexBuffer();
    vertexBuffer = geo.getVertexBuffer();
    indexCount = geo.getIndexCount();
}

void trc::Drawable::setMaterial(ui32 mat)
{
    material = mat;
}

auto trc::Drawable::getAnimationEngine() noexcept -> AnimationEngine&
{
    return animEngine;
}

void trc::Drawable::recordCommandBuffer(Deferred, vk::CommandBuffer cmdBuf)
{
    cmdBuf.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);
    cmdBuf.bindVertexBuffers(0, vertexBuffer, vk::DeviceSize(0));

    auto layout = GraphicsPipeline::at(Pipelines::eDrawableDeferred).getLayout();
    cmdBuf.pushConstants<mat4>(
        layout, vk::ShaderStageFlagBits::eVertex,
        0, getGlobalTransform()
    );
    cmdBuf.pushConstants<ui32>(
        layout, vk::ShaderStageFlagBits::eVertex,
        sizeof(mat4), material
    );
    animEngine.pushConstants(sizeof(mat4) + sizeof(ui32), layout, cmdBuf);

    cmdBuf.drawIndexed(indexCount, 1, 0, 0, 0);
}
