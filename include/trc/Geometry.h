#pragma once

#include <type_traits>
#include <optional>

#include "Types.h"
#include "Vertex.h"
#include "Rig.h"

namespace trc
{
    struct GeometryData
    {
        std::vector<Vertex> vertices;
        std::vector<VertexIndex> indices;
    };

    auto makePlaneGeo(
        float width = 1.0f,
        float height = 1.0f,
        ui32 segmentsX = 1,
        ui32 segmentsZ = 1,
        std::function<float(float, float)> heightFunc = [](...) { return 0.0f; }
    ) -> GeometryData;

    auto makeCubeGeo() -> GeometryData;

    /**
     * @brief Generate a sphere geometry
     *
     * @param size_t columns Number of vertices on any given horizontal
     *                       `2*PI`-long ring.
     * @param size_t rows    Number of vertices on any given vertical
     *                       `PI`-long half-ring. Is usually half as much
     *                       as `columns`.
     */
    auto makeSphereGeo(size_t columns = 32, size_t rows = 16) -> GeometryData;

    /**
     * @brief Handle to a geometry stored in the asset registry
     */
    class Geometry
    {
    private:
        friend class AssetRegistry;

        Geometry(vk::Buffer indices, ui32 numIndices, vk::IndexType indexType,
                 vk::Buffer verts, ui32 numVerts,
                 Rig* rig = nullptr);

    public:
        Geometry() = default;
        Geometry(const Geometry&) = default;
        Geometry(Geometry&&) noexcept = default;
        ~Geometry() = default;

        auto operator=(const Geometry&) -> Geometry& = default;
        auto operator=(Geometry&&) noexcept -> Geometry& = default;

        auto operator<=>(const Geometry& rhs) const -> std::strong_ordering = default;

        /**
         * @brief Bind vertex and index buffer
         *
         * @param vk::CommandBuffer cmdBuf
         * @param ui32 binding Binding index to which the vertex buffer will
         *                     be bound.
         */
        void bindVertices(vk::CommandBuffer cmdBuf, ui32 binding) const;

        auto getIndexBuffer() const noexcept -> vk::Buffer;
        auto getVertexBuffer() const noexcept -> vk::Buffer;
        auto getIndexCount() const noexcept -> ui32;
        auto getVertexCount() const noexcept -> ui32;

        bool hasRig() const;
        auto getRig() -> Rig*;

    private:
        vk::Buffer indexBuffer;
        vk::Buffer vertexBuffer;

        ui32 numIndices{ 0 };
        ui32 numVertices{ 0 };
        vk::IndexType indexType;

        Rig* rig;
    };

    static_assert(std::regular<Geometry>);
} // namespace trc
