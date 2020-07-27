#include "Geometry.h"



trc::Geometry::Geometry(const MeshData& data)
    :
    indexBuffer(data.indices, vk::BufferUsageFlagBits::eIndexBuffer, pool.makeAllocator()),
    vertexBuffer(data.vertices, vk::BufferUsageFlagBits::eVertexBuffer, pool.makeAllocator()),
    numIndices(data.indices.size())
{
}

trc::Geometry::Geometry(const MeshData& data, std::unique_ptr<Rig> rig)
    :
    Geometry(data)
{
    this->rig = std::move(rig);
}

auto trc::Geometry::getIndexBuffer() const noexcept -> vk::Buffer
{
    return *indexBuffer;
}

auto trc::Geometry::getVertexBuffer() const noexcept -> vk::Buffer
{
    return *vertexBuffer;
}

auto trc::Geometry::getIndexCount() const noexcept -> ui32
{
    return numIndices;
}

auto trc::Geometry::hasRig() const noexcept -> bool
{
    return rig != nullptr;
}

auto trc::Geometry::getRig() const noexcept -> Rig*
{
    return rig.get();
}
