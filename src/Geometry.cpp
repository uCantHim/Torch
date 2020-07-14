#include "Geometry.h"



trc::Geometry::Geometry(const MeshData& data)
    :
    indexBuffer(data.indices, vk::BufferUsageFlagBits::eIndexBuffer, pool.makeAllocator()),
    vertexBuffer(data.vertices, vk::BufferUsageFlagBits::eVertexBuffer, pool.makeAllocator()),
    numIndices(data.indices.size())
{
}

auto trc::Geometry::getIndexBuffer() const noexcept -> vk::Buffer
{
    return *indexBuffer;
}

auto trc::Geometry::getVertexBuffer() const noexcept -> vk::Buffer
{
    return *vertexBuffer;
}
