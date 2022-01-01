#pragma once

#include <vector>

#include "Types.h"

namespace trc
{
    struct MeshletDescription
    {
        ui32 vertexBegin;
        ui32 primitiveBegin;
        ui32 numVertices;
        ui32 numPrimVerts;
    };

    struct MeshletGeometry
    {
        std::vector<MeshletDescription> meshlets;

        std::vector<ui32> uniqueVertices;
        std::vector<ui8> primitiveIndices;
    };

    struct MeshletInfo
    {
        ui32 maxVertices{ 64 };
        ui32 maxPrimitives{ 126 };
    };

    auto makeMeshletIndices(const std::vector<ui32>& indexBuffer, const MeshletInfo& info = {})
        -> MeshletGeometry;
} // namespace trc
