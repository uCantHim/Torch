#include "Geometry.h"



trc::Geometry::Geometry(
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

void trc::Geometry::bindVertices(vk::CommandBuffer cmdBuf, ui32 binding) const
{
    cmdBuf.bindIndexBuffer(indexBuffer, 0, indexType);
    cmdBuf.bindVertexBuffers(binding, vertexBuffer, vk::DeviceSize(0));
}

auto trc::Geometry::getIndexBuffer() const noexcept -> vk::Buffer
{
    return indexBuffer;
}

auto trc::Geometry::getVertexBuffer() const noexcept -> vk::Buffer
{
    return vertexBuffer;
}

auto trc::Geometry::getIndexCount() const noexcept -> ui32
{
    return numIndices;
}

auto trc::Geometry::getVertexCount() const noexcept -> ui32
{
    return numVertices;
}

bool trc::Geometry::hasRig() const
{
    return rig != nullptr;
}

auto trc::Geometry::getRig() -> Rig*
{
    return rig;
}
