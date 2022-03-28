#pragma once

#include "Types.h"
#include "VulkanInclude.h"
#include "Assets.h"
#include "Rig.h"

namespace trc
{
    /**
     * @brief Handle to a geometry stored in the asset registry
     */
    class GeometryHandle
    {
    public:
        enum class VertexType : ui8
        {
            eMesh     = 1 << 0,
            eSkeletal = 1 << 1,
        };

        GeometryHandle() = default;
        GeometryHandle(const GeometryHandle&) = default;
        GeometryHandle(GeometryHandle&&) noexcept = default;
        ~GeometryHandle() = default;

        auto operator=(const GeometryHandle&) -> GeometryHandle& = default;
        auto operator=(GeometryHandle&&) noexcept -> GeometryHandle& = default;

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

        auto getIndexType() const noexcept -> vk::IndexType;
        auto getVertexType() const noexcept -> VertexType;
        auto getVertexSize() const noexcept -> size_t;

        bool hasRig() const;
        auto getRig() -> RigHandle;

    private:
        friend class GeometryRegistry;

        GeometryHandle(vk::Buffer indices, ui32 numIndices, vk::IndexType indexType,
                             vk::Buffer verts, VertexType vertexType,
                             std::optional<RigHandle> rig = std::nullopt);

        vk::Buffer indexBuffer;
        vk::Buffer vertexBuffer;

        ui32 numIndices{ 0 };
        vk::IndexType indexType;
        VertexType vertexType;

        std::optional<RigHandle> rig;
    };

    static_assert(std::semiregular<GeometryHandle>);
} // namespace trc
