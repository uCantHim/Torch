#include "Geometry.h"



auto trc::makePlaneGeo(
    float width,
    float height,
    ui32 segmentsX,
    ui32 segmentsY,
    std::function<float(float, float)> heightFunc) -> GeometryData
{
    GeometryData result;

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

auto trc::makeCubeGeo() -> GeometryData
{
    //return {
    //    {
    //        { { -0.5f, -0.5f, -0.5f }, normalize(vec3{ -1, -1, -1 }), { 0, 0 }, { 0, 0, 0 } },
    //        { { -0.5f, -0.5f,  0.5f }, normalize(vec3{ -1, -1,  1 }), { 0, 0 }, { 0, 0, 0 } },
    //        { { -0.5f,  0.5f, -0.5f }, normalize(vec3{ -1,  1, -1 }), { 0, 0 }, { 0, 0, 0 } },
    //        { { -0.5f,  0.5f,  0.5f }, normalize(vec3{ -1,  1,  1 }), { 0, 0 }, { 0, 0, 0 } },
    //        { {  0.5f, -0.5f, -0.5f }, normalize(vec3{  1, -1, -1 }), { 0, 0 }, { 0, 0, 0 } },
    //        /*5*/
    //        { {  0.5f, -0.5f,  0.5f }, normalize(vec3{  1, -1,  1 }), { 0, 0 }, { 0, 0, 0 } },
    //        { {  0.5f,  0.5f, -0.5f }, normalize(vec3{  1,  1, -1 }), { 0, 0 }, { 0, 0, 0 } },
    //        { {  0.5f,  0.5f,  0.5f }, normalize(vec3{  1,  1,  1 }), { 0, 0 }, { 0, 0, 0 } },
    //    },
    //    {
    //        // Left side
    //        0, 3, 2,
    //        0, 1, 3,
    //        // Front side
    //        1, 7, 3,
    //        1, 5, 7,
    //        // Right side
    //        5, 6, 7,
    //        5, 4, 6,
    //        // Back side
    //        4, 2, 6,
    //        4, 0, 2,
    //        // Top side
    //        3, 6, 2,
    //        3, 7, 6,
    //        // Bot side
    //        0, 5, 1,
    //        0, 4, 5,
    //    }
    //};

    return {
        {
            // Left side
            { { -0.5, -0.5, -0.5 }, normalize(vec3{ -1,  0,  0 }), { 0,     0.5  }, { 0, 0, -1 } },
            { { -0.5,  0.5,  0.5 }, normalize(vec3{ -1,  0,  0 }), { 0.333, 0.75 }, { 0, 0, -1 } },
            { { -0.5,  0.5, -0.5 }, normalize(vec3{ -1,  0,  0 }), { 0,     0.75 }, { 0, 0, -1 } },
            { { -0.5, -0.5, -0.5 }, normalize(vec3{ -1,  0,  0 }), { 0,     0.5  }, { 0, 0, -1 } },
            { { -0.5, -0.5,  0.5 }, normalize(vec3{ -1,  0,  0 }), { 0.333, 0.5  }, { 0, 0, -1 } },
            { { -0.5,  0.5,  0.5 }, normalize(vec3{ -1,  0,  0 }), { 0.333, 0.75 }, { 0, 0, -1 } },
            // Front side
            { { -0.5, -0.5,  0.5 }, normalize(vec3{  0,  0,  1 }), { 0.333, 0.5  }, { 1, 0, 0 } },
            { {  0.5,  0.5,  0.5 }, normalize(vec3{  0,  0,  1 }), { 0.666, 0.75 }, { 1, 0, 0 } },
            { { -0.5,  0.5,  0.5 }, normalize(vec3{  0,  0,  1 }), { 0.333, 0.75 }, { 1, 0, 0 } },
            { { -0.5, -0.5,  0.5 }, normalize(vec3{  0,  0,  1 }), { 0.333, 0.5  }, { 1, 0, 0 } },
            { {  0.5, -0.5,  0.5 }, normalize(vec3{  0,  0,  1 }), { 0.666, 0.5  }, { 1, 0, 0 } },
            { {  0.5,  0.5,  0.5 }, normalize(vec3{  0,  0,  1 }), { 0.666, 0.75 }, { 1, 0, 0 } },
            // Right side
            { {  0.5, -0.5,  0.5 }, normalize(vec3{  1,  0,  0 }), { 0.666, 0.5  }, { 0, 0, 1 } },
            { {  0.5,  0.5, -0.5 }, normalize(vec3{  1,  0,  0 }), { 1,     0.75 }, { 0, 0, 1 } },
            { {  0.5,  0.5,  0.5 }, normalize(vec3{  1,  0,  0 }), { 0.666, 0.75 }, { 0, 0, 1 } },
            { {  0.5, -0.5,  0.5 }, normalize(vec3{  1,  0,  0 }), { 0.666, 0.5  }, { 0, 0, 1 } },
            { {  0.5, -0.5, -0.5 }, normalize(vec3{  1,  0,  0 }), { 1,     0.5  }, { 0, 0, 1 } },
            { {  0.5,  0.5, -0.5 }, normalize(vec3{  1,  0,  0 }), { 1,     0.75 }, { 0, 0, 1 } },
            // Back side
            { {  0.5, -0.5, -0.5 }, normalize(vec3{  0,  0, -1 }), { 0.333, 0    }, { -1, 0, 0 } },
            { { -0.5,  0.5, -0.5 }, normalize(vec3{  0,  0, -1 }), { 0.666, 0.25 }, { -1, 0, 0 } },
            { {  0.5,  0.5, -0.5 }, normalize(vec3{  0,  0, -1 }), { 0.333, 0.25 }, { -1, 0, 0 } },
            { {  0.5, -0.5, -0.5 }, normalize(vec3{  0,  0, -1 }), { 0.333, 0    }, { -1, 0, 0 } },
            { { -0.5, -0.5, -0.5 }, normalize(vec3{  0,  0, -1 }), { 0.666, 0    }, { -1, 0, 0 } },
            { { -0.5,  0.5, -0.5 }, normalize(vec3{  0,  0, -1 }), { 0.666, 0.25 }, { -1, 0, 0 } },
            // Top side
            { { -0.5,  0.5,  0.5 }, normalize(vec3{  0,  1,  0 }), { 0.333, 0.75 }, { 1, 0, 0 } },
            { {  0.5,  0.5, -0.5 }, normalize(vec3{  0,  1,  0 }), { 0.666, 1    }, { 1, 0, 0 } },
            { { -0.5,  0.5, -0.5 }, normalize(vec3{  0,  1,  0 }), { 0.333, 1    }, { 1, 0, 0 } },
            { { -0.5,  0.5,  0.5 }, normalize(vec3{  0,  1,  0 }), { 0.333, 0.75 }, { 1, 0, 0 } },
            { {  0.5,  0.5,  0.5 }, normalize(vec3{  0,  1,  0 }), { 0.666, 0.75 }, { 1, 0, 0 } },
            { {  0.5,  0.5, -0.5 }, normalize(vec3{  0,  1,  0 }), { 0.666, 1    }, { 1, 0, 0 } },
            // Bot side
            { { -0.5, -0.5, -0.5 }, normalize(vec3{  0, -1,  0 }), { 0.333, 0.25 }, { -1, 0, 0 } },
            { {  0.5, -0.5,  0.5 }, normalize(vec3{  0, -1,  0 }), { 0.666, 0.5  }, { -1, 0, 0 } },
            { { -0.5, -0.5,  0.5 }, normalize(vec3{  0, -1,  0 }), { 0.333, 0.5  }, { -1, 0, 0 } },
            { { -0.5, -0.5, -0.5 }, normalize(vec3{  0, -1,  0 }), { 0.333, 0.25 }, { -1, 0, 0 } },
            { {  0.5, -0.5, -0.5 }, normalize(vec3{  0, -1,  0 }), { 0.666, 0.25 }, { -1, 0, 0 } },
            { {  0.5, -0.5,  0.5 }, normalize(vec3{  0, -1,  0 }), { 0.666, 0.5  }, { -1, 0, 0 } },
        },
        {
            0,  1,  2,  3,  4,  5,
            6,  7,  8,  9,  10, 11,
            12, 13, 14, 15, 16, 17,
            18, 19, 20, 21, 22, 23,
            24, 25, 26, 27, 28, 29,
            30, 31, 32, 33, 34, 35
        }
    };
}



trc::Geometry::Geometry(
    vk::Buffer indices,
    ui32 numIndices,
    vk::IndexType indexType,
    vk::Buffer verts,
    ui32 numVerts)
    :
    indexBuffer(indices),
    vertexBuffer(verts),
    numIndices(numIndices),
    numVertices(numVerts),
    indexType(indexType)
{
}

void trc::Geometry::bindVertices(vk::CommandBuffer cmdBuf, ui32 binding)
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
