#include "Drawable.h"

#include "AssetRegistry.h"
#include "Geometry.h"
#include "Material.h"



trc::Drawable::Drawable(Geometry& geo, Material& mat)
    :
    geometry(&geo),
    material(&mat)
{
}

auto trc::Drawable::getGeometry() const noexcept -> const Geometry&
{
    return *geometry;
}

auto trc::Drawable::getMaterial() const noexcept -> const Material&
{
    return *material;
}

void trc::Drawable::setGeometry(Geometry& geo)
{
    geometry = &geo;
}

void trc::Drawable::setMaterial(Material& mat)
{
    material = &mat;
}

void trc::Drawable::recordCommandBuffer(Deferred, vk::CommandBuffer cmdBuf)
{
    cmdBuf.bindIndexBuffer(geometry->getIndexBuffer(), 0, vk::IndexType::eUint32);
    cmdBuf.bindVertexBuffers(0, geometry->getVertexBuffer(), vk::DeviceSize(0));

    auto& pipeline = GraphicsPipeline::at(Pipelines::eDrawableDeferred);
    cmdBuf.pushConstants<mat4>(
        pipeline.getLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        0, getTransformationMatrix()
    );
    cmdBuf.pushConstants<ui32>(
        pipeline.getLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        sizeof(mat4), material->getAssetId()
    );

    cmdBuf.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, pipeline.getLayout(),
        0, AssetRegistry::getDescriptorSet(), {}
    );

    cmdBuf.drawIndexed(geometry->getIndexCount(), 1, 0, 0, 0);
}

void trc::Drawable::recordCommandBuffer(Lighting, vk::CommandBuffer cmdBuf)
{
}
