#include "ray_tracing/GeometryUtils.h"

#include "AssetRegistry.h"
#include "ray_tracing/AccelerationStructure.h"



auto trc::rt::makeGeometryInfo(const Geometry& geo) -> vk::AccelerationStructureGeometryKHR
{
    return { // Array of geometries in the AS
        vk::GeometryTypeKHR::eTriangles,
        vk::AccelerationStructureGeometryDataKHR{ // a union
            vk::AccelerationStructureGeometryTrianglesDataKHR(
                vk::Format::eR32G32B32Sfloat,
                vkb::getDevice()->getBufferAddress({ geo.getVertexBuffer() }),
                sizeof(trc::Vertex),
                geo.getIndexCount(), // max vertex
                vk::IndexType::eUint32,
                vkb::getDevice()->getBufferAddress({ geo.getIndexBuffer() }),
                nullptr // transform data
            )
        }
    };
}

trc::rt::GeometryInstance::GeometryInstance(
    glm::mat3x4 transform,
    const BottomLevelAccelerationStructure& blas)
    :
    transform(transform),
    accelerationStructureAddress(blas.getDeviceAddress())
{
}

trc::rt::GeometryInstance::GeometryInstance(
    mat4 transform,
    const BottomLevelAccelerationStructure& blas)
    :
    transform(transform),
    accelerationStructureAddress(blas.getDeviceAddress())
{
}

trc::rt::GeometryInstance::GeometryInstance(
    glm::mat3x4 transform,
    ui32 instanceCustomIndex,
    ui8 mask,
    ui32 shaderBindingTableRecordOffset,
    vk::GeometryInstanceFlagsKHR flags,
    const BottomLevelAccelerationStructure& blas)
    :
    transform(transform),
    instanceCustomIndex(instanceCustomIndex),
    mask(mask),
    shaderBindingTableRecordOffset(shaderBindingTableRecordOffset),
    flags(static_cast<ui32>(flags)),
    accelerationStructureAddress(blas.getDeviceAddress())
{
}

trc::rt::GeometryInstance::GeometryInstance(
    mat4 transform,
    ui32 instanceCustomIndex,
    ui8 mask,
    ui32 shaderBindingTableRecordOffset,
    vk::GeometryInstanceFlagsKHR flags,
    const BottomLevelAccelerationStructure& blas)
    :
    transform(transform),
    instanceCustomIndex(instanceCustomIndex),
    mask(mask),
    shaderBindingTableRecordOffset(shaderBindingTableRecordOffset),
    flags(static_cast<ui32>(flags)),
    accelerationStructureAddress(blas.getDeviceAddress())
{
}
