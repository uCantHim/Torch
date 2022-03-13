#pragma once

#include "Types.h"
#include "VulkanInclude.h"

namespace trc
{
    class Rig;

    /**
     * @brief Handle to a geometry stored in the asset registry
     */
    class GeometryDeviceHandle
    {
    public:
        enum class VertexType : ui8
        {
            eMesh     = 1 << 0,
            eSkeletal = 1 << 1,
        };

        GeometryDeviceHandle() = default;
        GeometryDeviceHandle(const GeometryDeviceHandle&) = default;
        GeometryDeviceHandle(GeometryDeviceHandle&&) noexcept = default;
        ~GeometryDeviceHandle() = default;

        auto operator=(const GeometryDeviceHandle&) -> GeometryDeviceHandle& = default;
        auto operator=(GeometryDeviceHandle&&) noexcept -> GeometryDeviceHandle& = default;

        auto operator<=>(const GeometryDeviceHandle& rhs) const -> std::strong_ordering = default;

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
        auto getRig() -> Rig*;

    private:
        friend class GeometryRegistry;

        GeometryDeviceHandle(vk::Buffer indices, ui32 numIndices, vk::IndexType indexType,
                             vk::Buffer verts, VertexType vertexType,
                             Rig* rig = nullptr);

        vk::Buffer indexBuffer;
        vk::Buffer vertexBuffer;

        ui32 numIndices{ 0 };
        vk::IndexType indexType;
        VertexType vertexType;

        Rig* rig;
    };

    static_assert(std::regular<GeometryDeviceHandle>);
} // namespace trc
