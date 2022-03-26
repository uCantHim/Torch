#include "Geometry.h"



trc::GeometryDeviceHandle::GeometryDeviceHandle(
    vk::Buffer indices,
    ui32 numIndices,
    vk::IndexType indexType,
    vk::Buffer verts,
    VertexType vertexType,
    std::optional<RigDeviceHandle> rig)
    :
    indexBuffer(indices),
    vertexBuffer(verts),
    numIndices(numIndices),
    indexType(indexType),
    vertexType(vertexType),
    rig(std::move(rig))
{
}

void trc::GeometryDeviceHandle::bindVertices(vk::CommandBuffer cmdBuf, ui32 binding) const
{
    cmdBuf.bindIndexBuffer(indexBuffer, 0, indexType);
    cmdBuf.bindVertexBuffers(binding, vertexBuffer, vk::DeviceSize(0));
}

auto trc::GeometryDeviceHandle::getIndexBuffer() const noexcept -> vk::Buffer
{
    return indexBuffer;
}

auto trc::GeometryDeviceHandle::getVertexBuffer() const noexcept -> vk::Buffer
{
    return vertexBuffer;
}

auto trc::GeometryDeviceHandle::getIndexCount() const noexcept -> ui32
{
    return numIndices;
}

auto trc::GeometryDeviceHandle::getIndexType() const noexcept -> vk::IndexType
{
    return indexType;
}

auto trc::GeometryDeviceHandle::getVertexType() const noexcept -> VertexType
{
    return vertexType;
}

bool trc::GeometryDeviceHandle::hasRig() const
{
    return rig.has_value();
}

auto trc::GeometryDeviceHandle::getRig() -> RigDeviceHandle
{
    return rig.value();
}
