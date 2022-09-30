#pragma once

#include <optional>
#include <mutex>
#include <unordered_set>

#include <vkb/Buffer.h>
#include <vkb/MemoryPool.h>
#include <trc_util/data/IndexMap.h>
#include <trc_util/data/ObjectId.h>

#include "util/DeviceLocalDataWriter.h"
#include "AssetRegistryModule.h"
#include "AssetSource.h"
#include "RigRegistry.h"
#include "SharedDescriptorSet.h"
#include "Vertex.h"

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
        SharedDescriptorSet::Builder& descriptorBuilder;

        vk::BufferUsageFlags geometryBufferUsage;
        bool enableRayTracing;

        ui32 memoryPoolChunkSize{ 200000000 };  // 200 MiB
        size_t maxGeometries{ 5000 };
    };

    /**
     * @brief
     */
    class GeometryRegistry : public AssetRegistryModuleCacheCrtpBase<Geometry>
    {
    public:
        enum class VertexType : ui8
        {
            eMesh     = 1 << 0,
            eSkeletal = 1 << 1,
        };

        using LocalID = TypedAssetID<Geometry>::LocalID;

        explicit GeometryRegistry(const GeometryRegistryCreateInfo& info);

        void update(vk::CommandBuffer cmdBuf, FrameRenderState& state) final;

        auto add(u_ptr<AssetSource<Geometry>> source) -> LocalID override;
        void remove(LocalID id) override;

        auto getHandle(LocalID id) -> AssetHandle<Geometry> override;

        void load(LocalID id) override;
        void unload(LocalID id) override;

        /**
         * @brief Create an acceleration structure for a geometry
         *
         * If an acceleration structure already exists for the given
         * geometry, the function does not create a new structure, but
         * returns the existing one.
         */
        auto makeAccelerationStructure(LocalID id) -> rt::BottomLevelAccelerationStructure&;

    private:
        friend class AssetHandle<Geometry>;

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
            struct DeviceData
            {
                vkb::DeviceLocalBuffer indexBuf;
                vkb::DeviceLocalBuffer vertexBuf;
                ui32 numIndices{ 0 };
                ui32 numVertices{ 0 };

                VertexType vertexType;
            };

            /** Declared as default in .cpp file for u_ptr to incomplete type BLAS */
            ~InternalStorage();

            ui32 deviceIndex;
            u_ptr<AssetSource<Geometry>> source;
            u_ptr<DeviceData> deviceData{ nullptr };
            std::optional<RigID> rig;

            u_ptr<rt::BottomLevelAccelerationStructure> blas{ nullptr };

            ReferenceCounter refCounter;
        };

        const Instance& instance;
        const Config config;

        data::IdPool idPool;
        vkb::MemoryPool memoryPool;
        DeviceLocalDataWriter dataWriter;

        std::mutex storageLock;
        data::IndexMap<LocalID::IndexType, u_ptr<InternalStorage>> storage;

        /**
         * Assets scheduled for removal from memory.
         * Buffers must only be destroyed after any deferred copy operations
         * have completed.
         */
        std::unordered_set<LocalID> pendingUnloads;

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
        using VertexType = GeometryRegistry::VertexType;

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
        auto getIndexCount() const noexcept -> ui32;

        auto getIndexType() const noexcept -> vk::IndexType;
        auto getVertexType() const noexcept -> VertexType;
        auto getVertexSize() const noexcept -> size_t;

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

    private:
        friend class GeometryRegistry;

        explicit AssetHandle(GeometryRegistry::SharedCacheReference ref,
                             GeometryRegistry::InternalStorage& data);

        GeometryRegistry::SharedCacheReference cacheRef;
        GeometryRegistry::InternalStorage* storage;
    };

    using GeometryHandle = AssetHandle<Geometry>;
    using GeometryData = AssetData<Geometry>;
    using GeometryID = TypedAssetID<Geometry>;
} // namespace trc
