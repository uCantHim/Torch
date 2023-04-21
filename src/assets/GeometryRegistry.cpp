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
    accelerationStructureBuilder(info.instance),
    deviceDataStorage(DeviceDataCache<DeviceData>::makeLoader(
        [this](ui32 id){ return loadDeviceData(LocalID{ id }); },
        [this](ui32 id, DeviceData data){ freeDeviceData(LocalID{ id }, std::move(data)); }
    )),
    indexDescriptorBinding(info.indexDescriptorBinding),
    vertexDescriptorBinding(info.vertexDescriptorBinding)
{
}

void GeometryRegistry::update(vk::CommandBuffer cmdBuf, FrameRenderState& frame)
{
    dataWriter.update(cmdBuf, frame);
    accelerationStructureBuilder.dispatchBuilds(cmdBuf, frame);

    pendingUnloads.clear();
}

auto GeometryRegistry::add(u_ptr<AssetSource<Geometry>> source) -> LocalID
{
    const LocalID id(idPool.generate());
    dataSources.emplace(id, std::move(source));

    return id;
}

void GeometryRegistry::remove(const LocalID id)
{
    dataSources.erase(id);
    idPool.free(id);
}

auto GeometryRegistry::getHandle(const LocalID id) -> GeometryHandle
{
    assert(dataSources.contains(id));
    return GeometryHandle{ deviceDataStorage.get(id) };
}

auto GeometryRegistry::loadDeviceData(const LocalID id) -> DeviceData
{
    assert(dataSources.contains(id));
    assert(dataSources.at(id) != nullptr);

    auto data = dataSources.at(id)->load();
    postProcess(id, data);

    const size_t indicesSize = data.indices.size() * sizeof(decltype(data.indices)::value_type);
    const size_t meshVerticesSize = data.vertices.size() * sizeof(decltype(data.vertices)::value_type);

    auto deviceData = DeviceData{
        .deviceIndex = ui32{id},
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
    };

    // Enqueue writes to the device-local vertex buffers
    dataWriter.write(*deviceData.indexBuf,      0, data.indices.data(),  indicesSize);
    dataWriter.write(*deviceData.meshVertexBuf, 0, data.vertices.data(), meshVerticesSize);

    if (!data.skeletalVertices.empty())
    {
        deviceData.hasSkeleton = true;

        const size_t skelVerticesSize = data.skeletalVertices.size()
                                        * sizeof(decltype(data.skeletalVertices)::value_type);
        deviceData.skeletalVertexBuf = {
            instance.getDevice(),
            skelVerticesSize, nullptr,
            config.geometryBufferUsage
                | vk::BufferUsageFlagBits::eVertexBuffer
                | vk::BufferUsageFlagBits::eTransferDst,
            memoryPool.makeAllocator()
        };
        dataWriter.write(*deviceData.skeletalVertexBuf, 0, data.skeletalVertices.data(), skelVerticesSize);
    }

    if (!data.rig.empty())
    {
        assert(!data.skeletalVertices.empty() && deviceData.hasSkeleton
                && "A geometry with a rig must also have a skeleton.");
        deviceData.rig = data.rig.getID();
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
                *deviceData.indexBuf, 0, VK_WHOLE_SIZE
            )
        );
        dataWriter.barrierPostWrite(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
            vk::BufferMemoryBarrier(
                vk::AccessFlagBits::eMemoryWrite,
                vk::AccessFlagBits::eShaderRead,
                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                *deviceData.meshVertexBuf, 0, VK_WHOLE_SIZE
            )
        );

        // Enqueue writes to the descriptor set
        const ui32 deviceIndex = deviceData.deviceIndex;
        vertexDescriptorBinding.update(deviceIndex, { *deviceData.meshVertexBuf, 0, VK_WHOLE_SIZE });
        indexDescriptorBinding.update(deviceIndex,  { *deviceData.indexBuf, 0, VK_WHOLE_SIZE });
    }

#ifdef TRC_DEBUG
    // Set some debug information
    auto meta = dataSources.at(id)->getMetadata();
    instance.getDevice().setDebugName(
        *deviceData.indexBuf,
        "Geometry index buffer for \"" + meta.name + "\""
        + " (local ID = " + std::to_string(id) + ")"
    );
    instance.getDevice().setDebugName(
        *deviceData.meshVertexBuf,
        "Geometry mesh-vertex buffer for \"" + meta.name + "\""
        + " (local ID = " + std::to_string(id) + ")"
    );
    if (deviceData.hasSkeleton)
    {
        instance.getDevice().setDebugName(
            *deviceData.skeletalVertexBuf,
            "Geometry skeletal-vertex buffer for \"" + meta.name + "\""
            + " (local ID = " + std::to_string(id) + ")"
        );
    }
#endif

    return deviceData;
}

void GeometryRegistry::freeDeviceData(LocalID /*id*/, DeviceData data)
{
    pendingUnloads.emplace_back(std::move(data));
}

void GeometryRegistry::postProcess(LocalID id, AssetData<Geometry>& data)
{
    try {
        data.indices = util::optimizeTriangleOrderingForsyth(data.indices);
    }
    catch (const std::invalid_argument& err) {
        log::warn << log::here() << ": Unable to optimize triangle order for geometry \""
                  << dataSources.at(id)->getMetadata().name << "\". (" << err.what() << ")";
    }
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
    auto handle = getHandle(id);
    auto geo = deviceDataStorage.get(id);
    if (!geo->blas)
    {
        geo->blas = std::make_unique<rt::BLAS>(
            instance,
            rt::makeGeometryInfo(instance.getDevice(), handle)
        );
        accelerationStructureBuilder.build(*geo->blas);
    }

    return *geo->blas;
}



GeometryHandle::AssetHandle(DeviceDataHandle deviceData)
    :
    deviceData(deviceData)
{
}

void GeometryHandle::bindVertices(vk::CommandBuffer cmdBuf, ui32 binding) const
{
    cmdBuf.bindIndexBuffer(*deviceData->indexBuf, 0, getIndexType());
    cmdBuf.bindVertexBuffers(binding, *deviceData->meshVertexBuf, vk::DeviceSize(0));
    if (hasSkeleton()) {
        cmdBuf.bindVertexBuffers(binding + 1, *deviceData->skeletalVertexBuf, vk::DeviceSize(0));
    }
}

auto GeometryHandle::getIndexBuffer() const noexcept -> vk::Buffer
{
    return *deviceData->indexBuf;
}

auto GeometryHandle::getVertexBuffer() const noexcept -> vk::Buffer
{
    return *deviceData->meshVertexBuf;
}

auto GeometryHandle::getSkeletalVertexBuffer() const noexcept -> vk::Buffer
{
    return *deviceData->skeletalVertexBuf;
}

auto GeometryHandle::getIndexCount() const noexcept -> ui32
{
    return deviceData->numIndices;
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
    return deviceData->hasSkeleton;
}

bool GeometryHandle::hasRig() const
{
    return deviceData->rig.has_value();
}

auto GeometryHandle::getRig() -> RigID
{
    if (!deviceData->rig.has_value())
    {
        throw std::out_of_range(
            "[In GeometryHandle::getRig()]: Geometry has no rig associated with it!"
        );
    }

    return deviceData->rig.value();
}

auto GeometryHandle::getDeviceIndex() const -> ui32
{
    return deviceData->deviceIndex;
}

bool GeometryHandle::hasAccelerationStructure() const
{
    return deviceData->blas != nullptr;
}

auto GeometryHandle::getAccelerationStructure() -> rt::BottomLevelAccelerationStructure&
{
    if (!hasAccelerationStructure())  {
        throw std::runtime_error("[In GeometryHandle::getAccelerationStructure]: The acceleration"
                                 " structure for this geometry has not been created.");
    }
    return *deviceData->blas;
}

auto GeometryHandle::getAccelerationStructure() const -> const rt::BottomLevelAccelerationStructure&
{
    if (!hasAccelerationStructure())  {
        throw std::runtime_error("[In GeometryHandle::getAccelerationStructure]: The acceleration"
                                 " structure for this geometry has not been created.");
    }
    return *deviceData->blas;
}

} // namespace trc
