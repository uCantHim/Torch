#include "GeometryRegistry.h"

#include "util/TriangleCacheOptimizer.h"
#include "ray_tracing/RayPipelineBuilder.h"



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
    std::vector<DescriptorLayoutBindingInfo> bindings;

    if (config.enableRayTracing)
    {
        bindings.push_back({
            config.vertBufBinding,
            vk::DescriptorType::eStorageBuffer,
            MAX_GEOMETRY_COUNT,
            rt::ALL_RAY_PIPELINE_STAGE_FLAGS,
            {}, vk::DescriptorBindingFlagBits::ePartiallyBound,
        });
        bindings.push_back({
            config.indexBufBinding,
            vk::DescriptorType::eStorageBuffer,
            MAX_GEOMETRY_COUNT,
            rt::ALL_RAY_PIPELINE_STAGE_FLAGS,
            {}, vk::DescriptorBindingFlagBits::ePartiallyBound,
        });
    }

    return bindings;
}

auto trc::GeometryRegistry::getDescriptorUpdates() -> std::vector<vk::WriteDescriptorSet>
{
    return {};
}

auto trc::GeometryRegistry::add(const GeometryData& data) -> LocalID
{
    LocalID id(idPool.generate());

    storage.emplace(
        static_cast<LocalID::Type>(id),
        InternalStorage{
            .indexBuf = {
                device,
                util::optimizeTriangleOrderingForsyth(data.indices),
                config.geometryBufferUsage | vk::BufferUsageFlagBits::eIndexBuffer,
                memoryPool.makeAllocator()
            },
            .vertexBuf = {
                device,
                data.vertices,
                config.geometryBufferUsage | vk::BufferUsageFlagBits::eVertexBuffer,
                memoryPool.makeAllocator()
            },
            .numIndices = static_cast<ui32>(data.indices.size()),
            .numVertices = static_cast<ui32>(data.vertices.size()),

            .rig = std::nullopt,
        }
    );

    return id;
}

void trc::GeometryRegistry::remove(LocalID id)
{
    idPool.free(static_cast<LocalID::Type>(id));
    storage.at(static_cast<LocalID::Type>(id)) = {};
}

auto trc::GeometryRegistry::getHandle(LocalID id) -> GeometryDeviceHandle
{
    return storage.at(static_cast<LocalID::Type>(id));
}
