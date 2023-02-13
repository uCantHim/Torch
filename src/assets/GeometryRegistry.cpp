#include "trc/assets/GeometryRegistry.h"

#include "geometry.pb.h"
#include "trc/assets/AssetManager.h"
#include "trc/assets/import/InternalFormat.h"
#include "trc/core/FrameRenderState.h"
#include "trc/ray_tracing/AccelerationStructure.h"
#include "trc/ray_tracing/GeometryUtils.h"
#include "trc/ray_tracing/RayPipelineBuilder.h"
#include "trc/util/TriangleCacheOptimizer.h"



namespace trc
{

void AssetData<Geometry>::serialize(std::ostream& os) const
{
    serial::Geometry geo = internal::serializeAssetData(*this);
    geo.SerializeToOstream(&os);
}

void AssetData<Geometry>::deserialize(std::istream& is)
{
    serial::Geometry geo;
    geo.ParseFromIstream(&is);
    *this = internal::deserializeAssetData(geo);
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
    cmdBuf.bindVertexBuffers(binding, *storage->deviceData->meshVertexBuf, vk::DeviceSize(0));
    if (hasSkeleton()) {
        cmdBuf.bindVertexBuffers(binding + 1, *storage->deviceData->skeletalVertexBuf, vk::DeviceSize(0));
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

auto GeometryHandle::getAccelerationStructure() const -> const rt::BottomLevelAccelerationStructure&
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

        return MemoryPool(info.instance.getDevice(), info.memoryPoolChunkSize, allocFlags);
    }()),
    dataWriter(info.instance.getDevice()),  /* , memoryPool.makeAllocator()) */
    accelerationStructureBuilder(info.instance)
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

void GeometryRegistry::update(vk::CommandBuffer cmdBuf, FrameRenderState& frame)
{
    dataWriter.update(cmdBuf, frame);
    accelerationStructureBuilder.dispatchBuilds(cmdBuf, frame);

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

    const size_t indicesSize = data.indices.size() * sizeof(decltype(data.indices)::value_type);
    const size_t meshVerticesSize = data.vertices.size() * sizeof(decltype(data.vertices)::value_type);

    auto& deviceData = item.deviceData;
    deviceData.reset(new InternalStorage::DeviceData{
        .indexBuf = {
            instance.getDevice(),
            indicesSize, nullptr,
            config.geometryBufferUsage
                | vk::BufferUsageFlagBits::eIndexBuffer
                | vk::BufferUsageFlagBits::eTransferDst,
            memoryPool.makeAllocator()
        },
        .meshVertexBuf = {
            instance.getDevice(),
            meshVerticesSize, nullptr,
            config.geometryBufferUsage
                | vk::BufferUsageFlagBits::eVertexBuffer
                | vk::BufferUsageFlagBits::eTransferDst,
            memoryPool.makeAllocator()
        },
        .skeletalVertexBuf = {},

        .numIndices = static_cast<ui32>(data.indices.size()),
        .numVertices = static_cast<ui32>(data.vertices.size()),
    });

    // Enqueue writes to the device-local vertex buffers
    dataWriter.write(*deviceData->indexBuf,      0, data.indices.data(),  indicesSize);
    dataWriter.write(*deviceData->meshVertexBuf, 0, data.vertices.data(), meshVerticesSize);

    if (!data.skeletalVertices.empty())
    {
        deviceData->hasSkeleton = true;

        const size_t skelVerticesSize = data.vertices.size() * sizeof(decltype(data.vertices)::value_type);
        deviceData->skeletalVertexBuf = {
            instance.getDevice(),
            skelVerticesSize, nullptr,
            config.geometryBufferUsage
                | vk::BufferUsageFlagBits::eVertexBuffer
                | vk::BufferUsageFlagBits::eTransferDst,
            memoryPool.makeAllocator()
        };
        dataWriter.write(*deviceData->skeletalVertexBuf, 0, data.skeletalVertices.data(), skelVerticesSize);
    }

    if (!data.rig.empty())
    {
        assert(!data.skeletalVertices.empty() && "A geometry with a rig must also have a skeleton.");
        item.rig = data.rig.getID();
    }

    if (config.enableRayTracing)
    {
        // Synchronize the deferred writes to vertex and index data with
        // reads during acceleration structure builds. The AS might not
        // even be built in the same frame/command buffer, but it is easier
        // always to add these barriers.
        //
        // Correct synchronization of build-input data buffers:
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#vkCmdBuildAccelerationStructuresKHR
        dataWriter.barrierPostWrite(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
            vk::BufferMemoryBarrier(
                vk::AccessFlagBits::eMemoryWrite,
                vk::AccessFlagBits::eShaderRead,
                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                *deviceData->indexBuf, 0, VK_WHOLE_SIZE
            )
        );
        dataWriter.barrierPostWrite(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
            vk::BufferMemoryBarrier(
                vk::AccessFlagBits::eMemoryWrite,
                vk::AccessFlagBits::eShaderRead,
                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                *deviceData->meshVertexBuf, 0, VK_WHOLE_SIZE
            )
        );

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
        data.blas = std::make_unique<rt::BLAS>(instance, rt::makeGeometryInfo(instance.getDevice(), geo));
        accelerationStructureBuilder.build(*data.blas);
    }

    return *data.blas;
}

} // namespace trc
