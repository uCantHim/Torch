#pragma once

#include "Types.h"

namespace trc
{
    class Rig;

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
