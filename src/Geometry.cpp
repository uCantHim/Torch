#include "Geometry.h"



trc::Geometry::Geometry(const MeshData& data)
    :
    indexBuffer(
        data.indices.size() * sizeof(uint32_t),
        data.indices.data(),
        vk::BufferUsageFlagBits::eIndexBuffer),
    vertexBuffer(
        data.vertices.size() * sizeof(Vertex),
        data.vertices.data(),
        vk::BufferUsageFlagBits::eVertexBuffer),
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
