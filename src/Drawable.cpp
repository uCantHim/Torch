#include "Drawable.h"

#include "AssetRegistry.h"
#include "Geometry.h"
#include "Material.h"



trc::Drawable::Drawable(Geometry& geo, ui32 mat)
    :
    geometry(&geo),
    material(mat)
{
}

trc::Drawable::Drawable(Geometry& geo, ui32 mat, SceneBase& scene)
    :
    Drawable(geo, mat)
{
    attachToScene(scene);
}

auto trc::Drawable::getGeometry() const noexcept -> const Geometry&
{
    return *geometry;
}

auto trc::Drawable::getMaterial() const noexcept -> ui32
{
    return material;
}

void trc::Drawable::setGeometry(Geometry& geo)
{
    geometry = &geo;
}

void trc::Drawable::setMaterial(ui32 mat)
{
    material = mat;
}

void trc::Drawable::recordCommandBuffer(Deferred, vk::CommandBuffer cmdBuf)
{
    cmdBuf.bindIndexBuffer(geometry->getIndexBuffer(), 0, vk::IndexType::eUint32);
    cmdBuf.bindVertexBuffers(0, geometry->getVertexBuffer(), vk::DeviceSize(0));

    auto& pipeline = GraphicsPipeline::at(Pipelines::eDrawableDeferred);
    cmdBuf.pushConstants<mat4>(
        pipeline.getLayout(), vk::ShaderStageFlagBits::eVertex,
        0, getGlobalTransform()
    );
    cmdBuf.pushConstants<ui32>(
        pipeline.getLayout(), vk::ShaderStageFlagBits::eVertex,
        sizeof(mat4), material
    );

    cmdBuf.drawIndexed(geometry->getIndexCount(), 1, 0, 0, 0);
}
