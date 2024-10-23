#pragma once

#include <optional>
#include <unordered_set>

#include <trc_util/data/IdPool.h>
#include <trc_util/data/SafeVector.h>

#include "trc/Vertex.h"
#include "trc/assets/AssetRegistryModule.h"
#include "trc/assets/AssetSource.h"
#include "trc/assets/DeviceDataCache.h"
#include "trc/assets/RigRegistry.h"
#include "trc/assets/SharedDescriptorSet.h"
#include "trc/base/Buffer.h"
#include "trc/base/MemoryPool.h"
#include "trc/util/AccelerationStructureBuilder.h"
#include "trc/util/DeviceLocalDataWriter.h"

namespace trc
{
    class GeometryRegistry;
    class Instance;
    namespace rt {
        class BottomLevelAccelerationStructure;
    }

    struct Geometry
    {
        using Registry = GeometryRegistry;

        static consteval auto name() -> std::string_view {
            return "torch_geo";
        }
    };

    template<>
    struct AssetData<Geometry>
    {
        std::vector<MeshVertex> vertices;
        std::vector<SkeletalVertex> skeletalVertices;
        std::vector<VertexIndex> indices;

        AssetReference<Rig> rig{};

        void resolveReferences(AssetManager& man);

        void serialize(std::ostream& os) const;
        void deserialize(std::istream& os);
    };

    struct GeometryRegistryCreateInfo
    {
        const Instance& instance;
        SharedDescriptorSet::Binding indexDescriptorBinding;
        SharedDescriptorSet::Binding vertexDescriptorBinding;

        vk::BufferUsageFlags geometryBufferUsage;
        bool enableRayTracing;

        ui32 memoryPoolChunkSize{ 200000000 };  // 200 MiB
        size_t maxGeometries{ 5000 };
    };

    /**
     * @brief
     */
    class GeometryRegistry : public AssetRegistryModuleInterface<Geometry>
    {
    public:
        using LocalID = TypedAssetID<Geometry>::LocalID;

        explicit GeometryRegistry(const GeometryRegistryCreateInfo& info);

        void update(vk::CommandBuffer cmdBuf, FrameRenderState& state) final;

        auto add(u_ptr<AssetSource<Geometry>> source) -> LocalID override;
        void remove(LocalID id) override;

        auto getHandle(LocalID id) -> AssetHandle<Geometry> override;

    private:
        friend class AssetHandle<Geometry>;

        struct Config
        {
            vk::BufferUsageFlags geometryBufferUsage;
            bool enableRayTracing;
        };

        struct DeviceData
        {
            ui32 deviceIndex;

            DeviceLocalBuffer indexBuf;
            DeviceLocalBuffer meshVertexBuf;
            DeviceLocalBuffer skeletalVertexBuf;
            ui32 numIndices{ 0 };
            ui32 numVertices{ 0 };

            bool hasSkeleton{ false };
            std::optional<RigID> rig{ std::nullopt };

            // Must be a unique_ptr instead of std::optional because rt::BLAS is
            // not move-constructible.
            u_ptr<rt::BottomLevelAccelerationStructure> blas{ nullptr };
        };

        auto loadDeviceData(LocalID id) -> DeviceData;
        void freeDeviceData(LocalID id, DeviceData data);

        void postProcess(LocalID id, AssetData<Geometry>& data);

        static auto makeAccelerationStructureGeometryInfo(const Device& device,
                                                          const DeviceData& data)
            -> vk::AccelerationStructureGeometryKHR;

        const Instance& instance;
        const Config config;

        data::IdPool<ui64> idPool;
        MemoryPool memoryPool;
        DeviceLocalDataWriter dataWriter;
        AccelerationStructureBuilder accelerationStructureBuilder;

        util::SafeVector<u_ptr<AssetSource<Geometry>>> dataSources;
        DeviceDataCache<DeviceData> deviceDataStorage;

        /**
         * Assets scheduled for removal from memory.
         * Buffers must only be destroyed after any deferred copy operations
         * have completed.
         */
        std::vector<DeviceData> pendingUnloads;

        SharedDescriptorSet::Binding indexDescriptorBinding;
        SharedDescriptorSet::Binding vertexDescriptorBinding;
    };

    /**
     * @brief Handle to a geometry stored in the asset registry
     */
    template<>
    class AssetHandle<Geometry>
    {
    public:
        AssetHandle() = delete;
        AssetHandle(const AssetHandle&) = default;
        AssetHandle(AssetHandle&&) noexcept = default;
        ~AssetHandle() = default;

        auto operator=(const AssetHandle&) -> AssetHandle& = default;
        auto operator=(AssetHandle&&) noexcept -> AssetHandle& = default;

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

        /**
         * @return vk::Buffer VK_NULL_HANDLE if the geometry does not have
         *         a skeleton.
         */
        auto getSkeletalVertexBuffer() const noexcept -> vk::Buffer;

        auto getIndexCount() const noexcept -> ui32;

        auto getIndexType() const noexcept -> vk::IndexType;
        auto getVertexSize() const noexcept -> size_t;
        auto getSkeletalVertexSize() const noexcept -> size_t;

        bool hasSkeleton() const;
        bool hasRig() const;
        auto getRig() -> RigID;

        /**
         * The device index is only meaningful when ray tracing is enabled.
         *
         * @return ui32 The descriptor index at which the geometry data can
         *         be accessed on the device.
         */
        auto getDeviceIndex() const -> ui32;

        /**
         * Create a geometry's acceleration structure with
         * GeometryRegistry::makeAccelerationStructure.
         */
        bool hasAccelerationStructure() const;

        /**
         * @throw std::runtime_error if hasAccelerationStructure() returns
         *        false.
         */
        auto getAccelerationStructure() -> rt::BottomLevelAccelerationStructure&;

        /**
         * @throw std::runtime_error if hasAccelerationStructure() returns
         *        false.
         */
        auto getAccelerationStructure() const -> const rt::BottomLevelAccelerationStructure&;

        auto getAccelerationStructureGeometry(const Device& device) const
            -> vk::AccelerationStructureGeometryKHR;

    private:
        friend class GeometryRegistry;
        using DeviceDataHandle = DeviceDataCache<GeometryRegistry::DeviceData>::CacheEntryHandle;

        explicit AssetHandle(DeviceDataHandle deviceData);

        DeviceDataHandle deviceData;
    };

    using GeometryHandle = AssetHandle<Geometry>;
    using GeometryData = AssetData<Geometry>;
    using GeometryID = TypedAssetID<Geometry>;
} // namespace trc
