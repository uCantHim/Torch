#include "assets/GeometryRegistry.h"

#include "geometry.pb.h"
#include "assets/AssetManager.h"
#include "assets/import/InternalFormat.h"
#include "util/TriangleCacheOptimizer.h"
#include "ray_tracing/AccelerationStructure.h"
#include "ray_tracing/RayPipelineBuilder.h"



namespace trc
{

void AssetData<Geometry>::serialize(std::ostream& os) const
{
    serial::Asset asset;
    *asset.mutable_geometry() = internal::serializeAssetData(*this);
    asset.SerializeToOstream(&os);
}

void AssetData<Geometry>::deserialize(std::istream& is)
{
    serial::Asset asset;
    asset.ParseFromIstream(&is);
    *this = internal::deserializeAssetData(asset.geometry());
}

void AssetData<Geometry>::resolveReferences(AssetManager& man)
{
    if (!rig.empty()) {
        rig.resolve(man);
    }
}



GeometryHandle::AssetHandle(
    GeometryRegistry::SharedCacheReference ref,
    GeometryRegistry::InternalStorage& _data)
    :
    cacheRef(std::move(ref)),
    storage(&_data)
{
    assert(storage->deviceData != nullptr);
}

void GeometryHandle::bindVertices(vk::CommandBuffer cmdBuf, ui32 binding) const
{
    cmdBuf.bindIndexBuffer(*storage->deviceData->indexBuf, 0, getIndexType());
    cmdBuf.bindVertexBuffers(binding, *storage->deviceData->meshVertexBuf, 0ul);
    if (hasSkeleton()) {
        cmdBuf.bindVertexBuffers(binding + 1, *storage->deviceData->skeletalVertexBuf, 0ul);
    }
}

auto GeometryHandle::getIndexBuffer() const noexcept -> vk::Buffer
{
    return *storage->deviceData->indexBuf;
}

auto GeometryHandle::getVertexBuffer() const noexcept -> vk::Buffer
{
    return *storage->deviceData->meshVertexBuf;
}

auto GeometryHandle::getSkeletalVertexBuffer() const noexcept -> vk::Buffer
{
    return *storage->deviceData->skeletalVertexBuf;
}

auto GeometryHandle::getIndexCount() const noexcept -> ui32
{
    return storage->deviceData->numIndices;
}

auto GeometryHandle::getIndexType() const noexcept -> vk::IndexType
{
    return vk::IndexType::eUint32;
}

auto GeometryHandle::getVertexSize() const noexcept -> size_t
{
    return sizeof(MeshVertex);
}

auto GeometryHandle::getSkeletalVertexSize() const noexcept -> size_t
{
    return sizeof(SkeletalVertex);
}

bool GeometryHandle::hasSkeleton() const
{
    return storage->deviceData->hasSkeleton;
}

bool GeometryHandle::hasRig() const
{
    return storage->rig.has_value();
}

auto GeometryHandle::getRig() -> RigID
{
    if (!storage->rig.has_value())
    {
        throw std::out_of_range(
            "[In GeometryHandle::getRig()]: Geometry has no rig associated with it!"
        );
    }

    return storage->rig.value();
}

auto GeometryHandle::getDeviceIndex() const -> ui32
{
    return storage->deviceIndex;
}

bool GeometryHandle::hasAccelerationStructure() const
{
    return storage->blas != nullptr;
}

auto GeometryHandle::getAccelerationStructure() -> rt::BottomLevelAccelerationStructure&
{
    if (!hasAccelerationStructure())  {
        throw std::runtime_error("[In GeometryHandle::getAccelerationStructure]: The acceleration"
                                 " structure for this geometry has not been created.");
    }
    return *storage->blas;
}



GeometryRegistry::InternalStorage::~InternalStorage() = default;

GeometryRegistry::GeometryRegistry(const GeometryRegistryCreateInfo& info)
    :
    instance(info.instance),
    config({ info.geometryBufferUsage, info.enableRayTracing }),
    memoryPool([&] {
        vk::MemoryAllocateFlags allocFlags;
        if (info.enableRayTracing) {
            allocFlags |= vk::MemoryAllocateFlagBits::eDeviceAddress;
        }

        return vkb::MemoryPool(info.instance.getDevice(), info.memoryPoolChunkSize, allocFlags);
    }()),
    dataWriter(info.instance.getDevice()) /* , memoryPool.makeAllocator()) */
{
    vertexDescriptorBinding = info.descriptorBuilder.addBinding(
        vk::DescriptorType::eStorageBuffer,
        info.maxGeometries,
        rt::ALL_RAY_PIPELINE_STAGE_FLAGS,
        vk::DescriptorBindingFlagBits::ePartiallyBound
            | vk::DescriptorBindingFlagBits::eUpdateAfterBind
    );
    indexDescriptorBinding = info.descriptorBuilder.addBinding(
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
        storage.at(id)->deviceData.reset();
    }
    pendingUnloads.clear();
}

auto GeometryRegistry::add(u_ptr<AssetSource<Geometry>> source) -> LocalID
{
    const LocalID id(idPool.generate());

    std::scoped_lock lock(storageLock);
    storage.emplace(
        id,
        new InternalStorage{
            .deviceIndex = id,
            .source = std::move(source),
            .deviceData = {},
            .rig = std::nullopt,
            .refCounter{ id, this }
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
    auto& data = *storage.at(id);
    return GeometryHandle(SharedCacheReference(data.refCounter), data);
}

void GeometryRegistry::load(const LocalID id)
{
    std::scoped_lock lock(storageLock);
    pendingUnloads.erase(id);

    auto& item = *storage.at(id);
    assert(item.source != nullptr);
    if (item.deviceData != nullptr)
    {
        // Is already loaded
        return;
    }

    auto data = item.source->load();
    data.indices = util::optimizeTriangleOrderingForsyth(data.indices);

    auto& deviceData = item.deviceData;
    deviceData.reset(new InternalStorage::DeviceData{
        .indexBuf = {
            instance.getDevice(),
            data.indices,
            config.geometryBufferUsage
                | vk::BufferUsageFlagBits::eIndexBuffer
                | vk::BufferUsageFlagBits::eTransferDst,
            memoryPool.makeAllocator()
        },
        .meshVertexBuf = {
            instance.getDevice(),
            data.vertices,
            config.geometryBufferUsage
                | vk::BufferUsageFlagBits::eVertexBuffer
                | vk::BufferUsageFlagBits::eTransferDst,
            memoryPool.makeAllocator()
        },
        .skeletalVertexBuf = {},

        .numIndices = static_cast<ui32>(data.indices.size()),
        .numVertices = static_cast<ui32>(data.vertices.size()),
    });

    if (!data.skeletalVertices.empty())
    {
        deviceData->hasSkeleton = true;
        deviceData->skeletalVertexBuf = {
            instance.getDevice(),
            data.skeletalVertices,
            config.geometryBufferUsage
                | vk::BufferUsageFlagBits::eVertexBuffer
                | vk::BufferUsageFlagBits::eTransferDst,
            memoryPool.makeAllocator()
        };
    }

    item.rig = data.rig.empty()
        ? std::optional<RigID>(std::nullopt)
        : data.rig.getID();

    // Enqueue writes to the device-local vertex buffers
    //const size_t indicesSize = data.indices.size() * sizeof(decltype(data.indices)::value_type);
    //const size_t meshVerticesSize = data.vertices.size() * sizeof(decltype(data.vertices)::value_type);
    //const size_t skelVerticesSize = data.vertices.size() * sizeof(decltype(data.vertices)::value_type);
    //dataWriter.write(*deviceData->indexBuf,          0, data.indices.data(),  indicesSize);
    //dataWriter.write(*deviceData->meshVertexBuf,     0, data.vertices.data(), meshVerticesSize);
    //dataWriter.write(*deviceData->skeletalVertexBuf, 0, data.vertices.data(), skelVerticesSize);

    if (config.enableRayTracing)
    {
        // Enqueue writes to the descriptor set
        const ui32 deviceIndex = item.deviceIndex;
        vertexDescriptorBinding.update(deviceIndex, { *deviceData->meshVertexBuf, 0, VK_WHOLE_SIZE });
        indexDescriptorBinding.update(deviceIndex,  { *deviceData->indexBuf, 0, VK_WHOLE_SIZE });
    }
}

void GeometryRegistry::unload(LocalID id)
{
    std::scoped_lock lock(storageLock);
    pendingUnloads.emplace(id);
}

auto GeometryRegistry::makeAccelerationStructure(LocalID id) -> rt::BLAS&
{
    if (!config.enableRayTracing)
    {
        throw std::invalid_argument("[In GeometryRegistry::makeAccelerationStructure]: Tried to"
                                    " query an acceleration structure, but ray tracing support is"
                                    " not enabled!");
    }

    // Get handle before acquiring the lock in case the geometry is not currently loaded.
    const auto geo = getHandle(id);

    std::scoped_lock lock(storageLock);
    auto& data = *storage.at(id);
    if (!data.blas)
    {
        data.blas = std::make_unique<rt::BLAS>(instance, geo);
        data.blas->build();
    }

    return *data.blas;
}

} // namespace trc
