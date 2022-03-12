#include "assets/GeneratedGeometry.h"



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
        .name = "",
        .vertices = {
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
        .skeletalVertices={},
        .indices = {
            0,  1,  2,  3,  4,  5,
            6,  7,  8,  9,  10, 11,
            12, 13, 14, 15, 16, 17,
            18, 19, 20, 21, 22, 23,
            24, 25, 26, 27, 28, 29,
            30, 31, 32, 33, 34, 35
        }
    };
}

auto trc::makeSphereGeo(const size_t cols, const size_t rows) -> GeometryData
{
    /////////////////////////
    //  Generate vertices  //
    /////////////////////////

    constexpr float pi{ glm::pi<float>() };
    constexpr float two_pi{ glm::two_pi<float>() };

    GeometryData data;
    auto genVertex = [&data](float rowAngle, float colAngle)
    {
        assert(rowAngle >= 0.0f && rowAngle <= pi);
        assert(colAngle >= 0.0f && colAngle <= two_pi);

        const float r = glm::sin(rowAngle);
        vec3 pos{ r * glm::cos(colAngle), glm::cos(rowAngle), r * glm::sin(colAngle) };
        vec3 normal{ glm::normalize(pos) };
        vec2 uv{};
        vec3 tangent{};

        data.vertices.emplace_back(MeshVertex(pos, normal, uv, tangent));
    };

    const float rowAngle{ pi / static_cast<float>(rows) };
    const float colAngle{ two_pi / static_cast<float>(cols) };

    genVertex(0.0f, 0.0f); // Bottom vertex
    for (size_t row = 1; row < rows; ++row)
    {
        for (size_t col = 0; col < cols; ++col)
        {
            genVertex(row * rowAngle, col * colAngle);
        }
    }
    genVertex(pi, 0.0f); // Top vertex


    ////////////////////////
    //  Generate indices  //
    ////////////////////////

    // Triangle fan from first single vertex to first full layer
    for (size_t i = 0; i < cols; i++)
    {
        data.indices.emplace_back(0);
        data.indices.emplace_back(i + 2 > cols ? 1 : i + 2);  // The wrap-around vertex
        data.indices.emplace_back(i + 1);
    }

    // We count triangle strips now
    //
    // A triangle has the following vertices:
    //   0: Second index of last primitive
    //   1: Neighbor of first index of last primitive
    //   2: First index of last primitive
    // 1 and 2 are swapped for every other primitive
    for (size_t baseRow = 0; baseRow < rows - 2; ++baseRow)
    {
        const ui32 rowStart = baseRow * cols + 1;  // +1 because 0 is single bottom point
        const ui32 nextRowStart = (baseRow + 1) * cols + 1;  // +1 because 0 is single bottom point
        const ui32 maxBaseRow = nextRowStart - 1;
        const ui32 maxNextRow = nextRowStart + cols - 1;

        ui32 base = rowStart;
        ui32 next = nextRowStart;
        for (size_t col = 0; col < cols; ++col, ++base, ++next)
        {
            data.indices.emplace_back(base);
            data.indices.emplace_back(next + 1 > maxNextRow ? nextRowStart : next + 1);
            data.indices.emplace_back(next);

            data.indices.emplace_back(next + 1 > maxNextRow ? nextRowStart : next + 1);
            data.indices.emplace_back(base);
            data.indices.emplace_back(base + 1 > maxBaseRow ? rowStart : base + 1);
        }
    }

    // Triangle fan from last full layer to last single vertex
    const ui32 lastVertex = data.vertices.size() - 1;
    const ui32 lastCol = lastVertex - cols;
    for (size_t i = 0; i < cols; i++)
    {
        data.indices.emplace_back(lastVertex);
        data.indices.emplace_back(lastCol + (i + 0));
        data.indices.emplace_back(lastCol + (i + 1 == cols ? 0 : i + 1));  // The wrap-around vertex
    }

    return data;
}
