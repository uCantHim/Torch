#include "trc/Meshlet.h"

#include <cstring>

#include <trc_util/Padding.h>

#include "trc/util/TriangleCacheOptimizer.h"



auto trc::makeMeshletIndices(const std::vector<ui32>& _indexBuffer, const MeshletInfo& info)
    -> MeshletGeometry
{
    constexpr ui32 PADDING = 8;

    auto indexBuffer = util::optimizeTriangleOrderingForsyth(_indexBuffer);
    assert(indexBuffer.size() % 3 == 0);

    MeshletGeometry geo;

    ui32 primitiveBegin{ 0 };
    ui32 numPrimVerts{ 0 };
    ui32 vertexBegin{ 0 };
    ui32 numVertices{ 0 };

    for (size_t i = 0; i < indexBuffer.size(); /* nothing */)
    {
        // Declare range of current meshlet's unique vertices
        auto begin = geo.uniqueVertices.begin() + vertexBegin;
        auto end = geo.uniqueVertices.end();
        auto findVertexIndex = [&](ui32 index) -> int
        {
            auto it = std::find(begin, end, index);
            return it == end ? -1 : it - begin;
        };

        int verts[] = {
            findVertexIndex(indexBuffer[i + 0]),
            findVertexIndex(indexBuffer[i + 1]),
            findVertexIndex(indexBuffer[i + 2]),
        };
        const ui32 newUniques = [&]{ ui32 n = 0; for (int i : verts) n += i < 0; return n; }();

        const ui32 paddedNumPrimVerts = util::pad(numPrimVerts, PADDING);
        // Create meshlet if size limitations have been reached
        if (paddedNumPrimVerts > info.maxPrimitives * 3    // This has been exceeded in the last loop
            || numVertices + newUniques > info.maxVertices)  // This would be exceeded this loop
        {
            assert(numPrimVerts % 3 == 0);

            // Add padding bytes
            for (ui32 i = 0; i < paddedNumPrimVerts - numPrimVerts; i++)
                geo.primitiveIndices.emplace_back(0);

            // Create meshlet
            geo.meshlets.emplace_back(MeshletDescription{
                .vertexBegin = vertexBegin,
                .primitiveBegin = primitiveBegin,
                .numVertices = numVertices,
                .numPrimVerts = numPrimVerts,
            });

            vertexBegin += numVertices;
            primitiveBegin += paddedNumPrimVerts;
            numVertices = 0;
            numPrimVerts = 0;
            continue;
        }

        // Add new unique indices
        for (size_t j = 0; j < 3; j++)
        {
            if (verts[j] < 0)
            {
                geo.uniqueVertices.emplace_back(indexBuffer[i + j]);
                ++numVertices;
                verts[j] = int((int(geo.uniqueVertices.size()) - 1) - vertexBegin);
            }
            assert(verts[j] >= 0 && verts[j] <= UINT8_MAX);
            assert(static_cast<ui32>(verts[j]) < numVertices);
            assert(static_cast<ui32>(verts[j]) < info.maxVertices);
            geo.primitiveIndices.emplace_back(static_cast<ui8>(verts[j]));
        }

        // Each loop adds one primitive
        numPrimVerts += 3;
        i += 3;  // Only go to next primitive if it has been added to a meshlet
    }

    if (numVertices > 0 && numPrimVerts > 0)
    {
        geo.meshlets.emplace_back(MeshletDescription{
            .vertexBegin=vertexBegin,
            .primitiveBegin=primitiveBegin,
            .numVertices=numVertices,
            .numPrimVerts=numPrimVerts
        });
    }

    return geo;
}

auto trc::makeMeshletVertices(
    const std::vector<MeshVertex>& vertices,
    const std::vector<SkeletalVertex>& skeletalVertices,
    const MeshletVertexDataInfo& info)
    -> MeshletVertexData
{
    const ui32 numVerts = vertices.size();
    const ui32 pad = info.bufferSectionAlignment;

    ui32 totalSize{ 0 };
    MeshletVertexData result{
        .positionOffset   = 0,
        .positionSize     = numVerts * static_cast<ui32>(sizeof(MeshVertex::position)),
        .uvOffset         = (totalSize += util::pad(result.positionSize, pad)),
        .uvSize           = numVerts * static_cast<ui32>(sizeof(MeshVertex::uv)),
        .normalOffset     = (totalSize += util::pad(result.uvSize, pad)),
        .normalSize       = numVerts * static_cast<ui32>(sizeof(MeshVertex::normal)),
        .tangentOffset    = (totalSize += util::pad(result.normalSize, pad)),
        .tangentSize      = numVerts * static_cast<ui32>(sizeof(MeshVertex::tangent)),
        .boneIndexOffset  = (totalSize += util::pad(result.tangentSize, pad)),
        .boneIndexSize    = numVerts * static_cast<ui32>(sizeof(SkeletalVertex::boneIndices)),
        .boneWeightOffset = (totalSize += util::pad(result.boneIndexSize, pad)),
        .boneWeightSize   = numVerts * static_cast<ui32>(sizeof(SkeletalVertex::boneWeights)),
        .data = {},
    };
    totalSize += result.boneWeightSize;
    result.data.resize(totalSize);

    ui8* data = result.data.data();
    for (ui32 i = 0; const auto& [p, n, uv, t] : vertices)
    {
        const auto& [bi, bw] = skeletalVertices.at(i);
        memcpy(data + result.positionOffset   + i * sizeof(p),  &p,  sizeof(p));
        memcpy(data + result.uvOffset         + i * sizeof(uv), &uv, sizeof(uv));
        memcpy(data + result.normalOffset     + i * sizeof(n),  &n,  sizeof(n));
        memcpy(data + result.tangentOffset    + i * sizeof(t),  &t,  sizeof(t));
        memcpy(data + result.boneIndexOffset  + i * sizeof(bi), &bi, sizeof(bi));
        memcpy(data + result.boneWeightOffset + i * sizeof(bw), &bw, sizeof(bw));
        ++i;
    }

    return result;
}
