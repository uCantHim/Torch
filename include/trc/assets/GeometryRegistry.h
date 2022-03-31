#pragma once

#include <optional>

#include <vkb/Buffer.h>
#include <vkb/MemoryPool.h>
#include <trc_util/data/IndexMap.h>
#include <trc_util/data/ObjectId.h>

#include "AssetBaseTypes.h"
#include "AssetRegistryModule.h"
#include "AssetSource.h"
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

    /**
     * @brief
     */
    class GeometryRegistry : public AssetRegistryModuleInterface
    {
    public:
        using LocalID = TypedAssetID<Geometry>::LocalID;
        using Handle = GeometryHandle;

        explicit GeometryRegistry(const AssetRegistryModuleCreateInfo& info);

        void update(vk::CommandBuffer cmdBuf) final;

        auto add(u_ptr<AssetSource<Geometry>> source) -> LocalID;
        void remove(LocalID id);

        auto getHandle(LocalID id) -> GeometryHandle;

    private:
        struct Config
        {
            vk::BufferUsageFlags geometryBufferUsage;
            bool enableRayTracing;
        };

        /**
         * GPU resources for geometry data
         */
        struct InternalStorage
        {
            using VertexType = GeometryHandle::VertexType;

            operator GeometryHandle();

            vkb::DeviceLocalBuffer indexBuf;
            vkb::DeviceLocalBuffer vertexBuf;
            ui32 numIndices{ 0 };
            ui32 numVertices{ 0 };

            VertexType vertexType;

            std::optional<RigHandle> rig;

            ui32 deviceIndex;
        };

        static constexpr ui32 MEMORY_POOL_CHUNK_SIZE = 200000000;  // 200 MiB
        static constexpr ui32 MAX_GEOMETRY_COUNT = 5000;

        const vkb::Device& device;
        const Config config;

        vkb::MemoryPool memoryPool;
        data::IdPool idPool;

        data::IndexMap<LocalID::IndexType, InternalStorage> storage;

        SharedDescriptorSet::Binding indexDescriptorBinding;
        SharedDescriptorSet::Binding vertexDescriptorBinding;
    };
} // namespace trc
