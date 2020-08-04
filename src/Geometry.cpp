#include "Geometry.h"



auto trc::makePlaneGeo(
    float width,
    float height,
    ui32 segmentsX,
    ui32 segmentsY,
    std::function<float(float, float)> heightFunc) -> MeshData
{
    MeshData result;

    const float sizeX = width / segmentsX;
    const float sizeY = height / segmentsY;

    // Create vertices
    float yCoord = -height * 0.5f;
    for (ui32 y = 0; y < segmentsY + 1; y++, yCoord += sizeY)
    {
        float xCoord = -width * 0.5f;
        for (ui32 x = 0; x < segmentsX + 1; x++, xCoord += sizeX)
        {
            vec3 pos{ xCoord, heightFunc(xCoord, yCoord), yCoord };
            vec3 normal{ 0.0f, 1.0f, 0.0f };
            vec3 tangent{ 1.0f, 0.0f, 0.0f };
            vec2 uv{ static_cast<float>(x), static_cast<float>(y) };

            result.vertices.emplace_back(pos, normal, uv, tangent);
        }
    }

    // Fill indices
    const ui32 rowSize = segmentsX + 1;
    for (ui32 y = 0; y < segmentsY; y++)
    {
        for (ui32 x = 0; x < segmentsX; x++)
        {
            const ui32 base = y * rowSize;
            std::vector<ui32> indices{
                // Lower-left triangle
                base + x,
                base + x + rowSize,
                base + x + 1,
                // Upper-right triangle
                base + x + 1,
                base + x + rowSize,
                base + x + 1 + rowSize,
            };

            result.indices.insert(result.indices.end(), indices.begin(), indices.end());
        }
    }

    return result;
}



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
