#include "Geometry.h"



trc::GeometryHandle::GeometryHandle(
    vk::Buffer indices,
    ui32 numIndices,
    vk::IndexType indexType,
    vk::Buffer verts,
    VertexType vertexType,
    std::optional<RigHandle> rig)
    :
    indexBuffer(indices),
    vertexBuffer(verts),
    numIndices(numIndices),
    indexType(indexType),
    vertexType(vertexType),
    rig(std::move(rig))
{
}

void trc::GeometryHandle::bindVertices(vk::CommandBuffer cmdBuf, ui32 binding) const
{
    cmdBuf.bindIndexBuffer(indexBuffer, 0, indexType);
    cmdBuf.bindVertexBuffers(binding, vertexBuffer, vk::DeviceSize(0));
}

auto trc::GeometryHandle::getIndexBuffer() const noexcept -> vk::Buffer
{
    return indexBuffer;
}

auto trc::GeometryHandle::getVertexBuffer() const noexcept -> vk::Buffer
{
    return vertexBuffer;
}

auto trc::GeometryHandle::getIndexCount() const noexcept -> ui32
{
    return numIndices;
}

auto trc::GeometryHandle::getIndexType() const noexcept -> vk::IndexType
{
    return indexType;
}

auto trc::GeometryHandle::getVertexType() const noexcept -> VertexType
{
    return vertexType;
}

bool trc::GeometryHandle::hasRig() const
{
    return rig.has_value();
}

auto trc::GeometryHandle::getRig() -> RigHandle
{
    return rig.value();
}
