#pragma once

#include <vector>

#include "trc/Types.h"
#include "trc/Vertex.h"

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

    struct MeshletVertexData
    {
        ui32 positionOffset;
        ui32 positionSize;
        ui32 uvOffset;
        ui32 uvSize;
        ui32 normalOffset;
        ui32 normalSize;
        ui32 tangentOffset;
        ui32 tangentSize;

        ui32 boneIndexOffset;
        ui32 boneIndexSize;
        ui32 boneWeightOffset;
        ui32 boneWeightSize;

        std::vector<ui8> data;
    };

    struct MeshletVertexDataInfo
    {
        ui32 bufferSectionAlignment{ 16 };
    };

    auto makeMeshletVertices(const std::vector<MeshVertex>& vertices,
                             const std::vector<SkeletalVertex>& skeletalVertices,
                             const MeshletVertexDataInfo& info = {})
        -> MeshletVertexData;
} // namespace trc
