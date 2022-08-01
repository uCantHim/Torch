#include "assets/GeometryRegistry.h"

#include "util/TriangleCacheOptimizer.h"
#include "ray_tracing/RayPipelineBuilder.h"
#include "assets/AssetManager.h"



namespace trc
{

auto makeVertexData(const GeometryData& geo) -> std::vector<ui8>
{
    assert(geo.skeletalVertices.empty()
           || geo.skeletalVertices.size() == geo.vertices.size());

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



GeometryHandle::AssetHandle(GeometryRegistry::CacheItemRef ref)
    :
    cacheRef(std::move(ref)),
    data(cacheRef->deviceData.get())
{
    assert(data != nullptr);
}

void GeometryHandle::bindVertices(vk::CommandBuffer cmdBuf, ui32 binding) const
{
    cmdBuf.bindIndexBuffer(*data->indexBuf, 0, getIndexType());
    cmdBuf.bindVertexBuffers(binding, *data->vertexBuf, vk::DeviceSize(0));
}

auto GeometryHandle::getIndexBuffer() const noexcept -> vk::Buffer
{
    return *data->indexBuf;
}

auto GeometryHandle::getVertexBuffer() const noexcept -> vk::Buffer
{
    return *data->vertexBuf;
}

auto GeometryHandle::getIndexCount() const noexcept -> ui32
{
    return data->numIndices;
}

auto GeometryHandle::getIndexType() const noexcept -> vk::IndexType
{
    return vk::IndexType::eUint32;
}

auto GeometryHandle::getVertexType() const noexcept -> VertexType
{
    return data->vertexType;
}

bool GeometryHandle::hasRig() const
{
    return cacheRef->rig.has_value();
}

auto GeometryHandle::getRig() -> RigID
{
    if (!cacheRef->rig.has_value())
    {
        throw std::out_of_range(
            "[In GeometryHandle::getRig()]: Geometry has no rig associated with it!"
        );
    }

    return cacheRef->rig.value();
}



GeometryRegistry::GeometryRegistry(const GeometryRegistryCreateInfo& info)
    :
    device(info.device),
    config({ info.geometryBufferUsage, info.enableRayTracing }),
    memoryPool([&] {
        vk::MemoryAllocateFlags allocFlags;
        if (info.enableRayTracing) {
            allocFlags |= vk::MemoryAllocateFlagBits::eDeviceAddress;
        }

        return vkb::MemoryPool(info.device, info.memoryPoolChunkSize, allocFlags);
    }()),
    dataWriter(info.device) /* , memoryPool.makeAllocator()) */
{
    indexDescriptorBinding = info.descriptorBuilder.addBinding(
        vk::DescriptorType::eStorageBuffer,
        info.maxGeometries,
        rt::ALL_RAY_PIPELINE_STAGE_FLAGS,
        vk::DescriptorBindingFlagBits::ePartiallyBound
            | vk::DescriptorBindingFlagBits::eUpdateAfterBind
    );
    vertexDescriptorBinding = info.descriptorBuilder.addBinding(
        vk::DescriptorType::eStorageBuffer,
        info.maxGeometries,
        rt::ALL_RAY_PIPELINE_STAGE_FLAGS,
        vk::DescriptorBindingFlagBits::ePartiallyBound
            | vk::DescriptorBindingFlagBits::eUpdateAfterBind
    );
}

void GeometryRegistry::update(vk::CommandBuffer cmdBuf, FrameRenderState& state)
{
    dataWriter.update(cmdBuf, state);

    std::scoped_lock lock(storageLock);
    for (auto id : pendingUnloads) {
        storage.at(id)->getItem().deviceData.reset();
    }
    pendingUnloads.clear();
}

auto GeometryRegistry::add(u_ptr<AssetSource<Geometry>> source) -> LocalID
{
    const LocalID id(idPool.generate());

    std::scoped_lock lock(storageLock);
    storage.emplace(
        id,
        new CacheItem<InternalStorage>{
            InternalStorage{
                .deviceIndex = id,
                .source = std::move(source),
                .deviceData = {},
                .rig = std::nullopt
            },
            id, this
        }
    );

    return id;
}

void GeometryRegistry::remove(const LocalID id)
{
    std::scoped_lock lock(storageLock);
    idPool.free(id);
    storage.at(id).reset();
}

auto GeometryRegistry::getHandle(const LocalID id) -> GeometryHandle
{
    assert(storage.at(id) != nullptr);
    return GeometryHandle(CacheItemRef(*storage.at(id)));
}

void GeometryRegistry::load(const LocalID id)
{
    std::scoped_lock lock(storageLock);
    pendingUnloads.erase(id);

    auto& item = storage.at(id)->getItem();
    assert(item.source != nullptr);
    if (item.deviceData != nullptr)
    {
        // Is already loaded
        return;
    }

    auto data = item.source->load();
    data.indices = util::optimizeTriangleOrderingForsyth(data.indices);
    const auto vertexData = makeVertexData(data);
    const size_t indicesSize = data.indices.size() * sizeof(decltype(data.indices)::value_type);
    const size_t verticesSize = vertexData.size() * sizeof(decltype(vertexData)::value_type);

    auto& deviceData = item.deviceData;
    deviceData.reset(new InternalStorage::DeviceData{
        .indexBuf = {
            device,
            indicesSize,
            config.geometryBufferUsage
                | vk::BufferUsageFlagBits::eIndexBuffer
                | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            memoryPool.makeAllocator()
        },
        .vertexBuf = {
            device,
            verticesSize,
            config.geometryBufferUsage
                | vk::BufferUsageFlagBits::eVertexBuffer
                | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            memoryPool.makeAllocator()
        },
        .numIndices = static_cast<ui32>(data.indices.size()),
        .numVertices = static_cast<ui32>(data.vertices.size()),

        .vertexType = data.skeletalVertices.empty()
            ? VertexType::eMesh
            : VertexType::eSkeletal,
    });
    item.rig = data.rig.empty()
        ? std::optional<RigID>(std::nullopt)
        : data.rig.getID();

    // Enqueue writes to the device-local vertex buffers
    dataWriter.write(*deviceData->indexBuf, 0, data.indices.data(), indicesSize);
    dataWriter.write(*deviceData->vertexBuf, 0, vertexData.data(), verticesSize);

    if (config.enableRayTracing)
    {
        // Enqueue writes to the descriptor set
        const ui32 deviceIndex = item.deviceIndex;
        indexDescriptorBinding.update(deviceIndex,  { *deviceData->indexBuf, 0, VK_WHOLE_SIZE });
        vertexDescriptorBinding.update(deviceIndex, { *deviceData->vertexBuf, 0, VK_WHOLE_SIZE });
    }
}

void GeometryRegistry::unload(LocalID id)
{
    std::scoped_lock lock(storageLock);
    pendingUnloads.emplace(id);
}

} // namespace trc
