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
    cmdBuf.bindVertexBuffers(binding, *storage->deviceData->vertexBuf, vk::DeviceSize(0));
}

auto GeometryHandle::getIndexBuffer() const noexcept -> vk::Buffer
{
    return *storage->deviceData->indexBuf;
}

auto GeometryHandle::getVertexBuffer() const noexcept -> vk::Buffer
{
    return *storage->deviceData->vertexBuf;
}

auto GeometryHandle::getIndexCount() const noexcept -> ui32
{
    return storage->deviceData->numIndices;
}

auto GeometryHandle::getIndexType() const noexcept -> vk::IndexType
{
    return vk::IndexType::eUint32;
}

auto GeometryHandle::getVertexType() const noexcept -> VertexType
{
    return storage->deviceData->vertexType;
}

auto GeometryHandle::getVertexSize() const noexcept -> size_t
{
    return getVertexType() == VertexType::eMesh ? sizeof(MeshVertex)
                                                : sizeof(MeshVertex) + sizeof(SkeletalVertex);
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
    const auto vertexData = makeVertexData(data);
    const size_t indicesSize = data.indices.size() * sizeof(decltype(data.indices)::value_type);
    const size_t verticesSize = vertexData.size() * sizeof(decltype(vertexData)::value_type);

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
        .vertexBuf = {
            instance.getDevice(),
            vertexData,
            config.geometryBufferUsage
                | vk::BufferUsageFlagBits::eVertexBuffer
                | vk::BufferUsageFlagBits::eTransferDst,
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
    //dataWriter.write(*deviceData->indexBuf, 0, data.indices.data(), indicesSize);
    //dataWriter.write(*deviceData->vertexBuf, 0, vertexData.data(), verticesSize);

    if (config.enableRayTracing)
    {
        // Enqueue writes to the descriptor set
        const ui32 deviceIndex = item.deviceIndex;
        vertexDescriptorBinding.update(deviceIndex, { *deviceData->vertexBuf, 0, VK_WHOLE_SIZE });
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
