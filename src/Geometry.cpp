#include "Geometry.h"



trc::GeometryDeviceHandle::GeometryDeviceHandle(
    vk::Buffer indices,
    ui32 numIndices,
    vk::IndexType indexType,
    vk::Buffer verts,
    ui32 numVerts,
    Rig* rig)
    :
    indexBuffer(indices),
    vertexBuffer(verts),
    numIndices(numIndices),
    numVertices(numVerts),
    indexType(indexType),
    rig(rig)
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

auto trc::GeometryDeviceHandle::getVertexCount() const noexcept -> ui32
{
    return numVertices;
}

bool trc::GeometryDeviceHandle::hasRig() const
{
    return rig != nullptr;
}

auto trc::GeometryDeviceHandle::getRig() -> Rig*
{
    return rig;
}
