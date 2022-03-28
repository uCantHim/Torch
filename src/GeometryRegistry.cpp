#include "GeometryRegistry.h"

#include "util/TriangleCacheOptimizer.h"
#include "ray_tracing/RayPipelineBuilder.h"
#include "AssetManager.h"



namespace trc
{
    auto makeVertexData(const GeometryData& geo) -> std::vector<ui8>
    {
        assert(geo.skeletalVertices.empty()
               || geo.skeletalVertices.size() == geo.vertices.size());
        // assert(geo.skeletalVertices.empty() == !geo.rig.has_value());

        std::vector<ui8> result;
        result.resize(geo.vertices.size() * sizeof(MeshVertex)
                      + geo.skeletalVertices.size() * sizeof(SkeletalVertex));

        const bool hasSkel = !geo.skeletalVertices.empty();
        const size_t vertSize = sizeof(MeshVertex) + hasSkel * sizeof(SkeletalVertex);
        for (size_t i = 0; i < geo.vertices.size(); i++)
        {
            const size_t offset = i * vertSize;
            memcpy(result.data() + offset, &geo.vertices.at(i), sizeof(MeshVertex));

            if (hasSkel)
            {
                memcpy(result.data() + offset + sizeof(MeshVertex),
                       &geo.skeletalVertices.at(i),
                       sizeof(SkeletalVertex));
            }
        }

        return result;
    }
}



trc::GeometryRegistry::InternalStorage::operator GeometryHandle()
{
    return {
        *indexBuf, numIndices, vk::IndexType::eUint32,
        *vertexBuf, vertexType,
        rig.has_value() ? rig.value() : std::optional<RigHandle>{ std::nullopt }
    };
}



trc::GeometryRegistry::GeometryRegistry(const AssetRegistryModuleCreateInfo& info)
    :
    device(info.device),
    config({
        info.geometryBufferUsage,
        info.enableRayTracing,
        info.geoVertexBufBinding,
        info.geoIndexBufBinding
    }),
    memoryPool([&] {
        vk::MemoryAllocateFlags allocFlags;
        if (info.enableRayTracing) {
            allocFlags |= vk::MemoryAllocateFlagBits::eDeviceAddress;
        }

        return vkb::MemoryPool(info.device, MEMORY_POOL_CHUNK_SIZE, allocFlags);
    }())
{
}

void trc::GeometryRegistry::update(vk::CommandBuffer)
{
    // Nothing
}

auto trc::GeometryRegistry::getDescriptorLayoutBindings()
    -> std::vector<DescriptorLayoutBindingInfo>
{
    return {
        {
            config.vertBufBinding,
            vk::DescriptorType::eStorageBuffer,
            MAX_GEOMETRY_COUNT,
            rt::ALL_RAY_PIPELINE_STAGE_FLAGS,
            {}, vk::DescriptorBindingFlagBits::ePartiallyBound,
        },
        {
            config.indexBufBinding,
            vk::DescriptorType::eStorageBuffer,
            MAX_GEOMETRY_COUNT,
            rt::ALL_RAY_PIPELINE_STAGE_FLAGS,
            {}, vk::DescriptorBindingFlagBits::ePartiallyBound,
        },
    };
}

auto trc::GeometryRegistry::getDescriptorUpdates() -> std::vector<vk::WriteDescriptorSet>
{
    return {};
}

auto trc::GeometryRegistry::add(u_ptr<AssetSource<Geometry>> source) -> LocalID
{
    LocalID id(idPool.generate());

    auto data = source->load();

    storage.emplace(
        static_cast<LocalID::IndexType>(id),
        InternalStorage{
            .indexBuf = {
                device,
                util::optimizeTriangleOrderingForsyth(data.indices),
                config.geometryBufferUsage | vk::BufferUsageFlagBits::eIndexBuffer,
                memoryPool.makeAllocator()
            },
            .vertexBuf = {
                device,
                makeVertexData(data),
                config.geometryBufferUsage | vk::BufferUsageFlagBits::eVertexBuffer,
                memoryPool.makeAllocator()
            },
            .numIndices = static_cast<ui32>(data.indices.size()),
            .numVertices = static_cast<ui32>(data.vertices.size()),

            .vertexType = data.skeletalVertices.empty()
                ? InternalStorage::VertexType::eMesh
                : InternalStorage::VertexType::eSkeletal,

            .rig = data.rig.empty()
                ? std::optional<AssetHandle<Rig>>(std::nullopt)
                : data.rig.getID().getDeviceDataHandle(),

            .deviceIndex = id,
        }
    );

    return id;
}

void trc::GeometryRegistry::remove(LocalID id)
{
    idPool.free(static_cast<LocalID::IndexType>(id));
    storage.at(static_cast<LocalID::IndexType>(id)) = {};
}

auto trc::GeometryRegistry::getHandle(LocalID id) -> GeometryHandle
{
    return storage.at(static_cast<LocalID::IndexType>(id));
}
