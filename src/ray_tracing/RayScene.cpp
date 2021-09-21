#include "ray_tracing/RayScene.h"

#include "core/Instance.h"
#include "AssetRegistry.h"
#include "ray_tracing/GeometryUtils.h"



trc::rt::RayScene::RayScene(const Instance& instance)
    :
    instance(instance),
    device(instance.getDevice()),
    blasMemoryPool(device, 100000000), // 100 MB
    blasAlloc(blasMemoryPool.makeAllocator()),
    blasInstanceBuffer(
        device,
        MAX_TLAS_INSTANCES * sizeof(GeometryInstance),
        vk::BufferUsageFlagBits::eShaderDeviceAddress
        | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    blasInstanceBufferMap(reinterpret_cast<GeometryInstance*>(blasInstanceBuffer.map())),
    tlas(instance, MAX_TLAS_INSTANCES),
    drawableDataStagingBuffer(
        device,
        MAX_TLAS_INSTANCES * sizeof(DrawableShadingData),
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    ),
    drawableDataDeviceBuffer(
        device,
        MAX_TLAS_INSTANCES * sizeof(DrawableShadingData),
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    ),
    drawableDataBufferMap(reinterpret_cast<DrawableShadingData*>(drawableDataStagingBuffer.map()))
{
}

void trc::rt::RayScene::update(vk::CommandBuffer cmdBuf)
{
    // Update BLAS transforms
    for (ui32 i = 0; i < modelMatrices.size(); i++)
    {
        blasInstanceBufferMap[i].transform = glm::transpose(modelMatrices.at(i).get());
    }

    cmdBuf.copyBuffer(
        *drawableDataStagingBuffer, *drawableDataDeviceBuffer,
        vk::BufferCopy(0, 0, drawables.size() * sizeof(DrawableShadingData))
    );

    tlas.build(cmdBuf, *blasInstanceBuffer, drawables.size());
}

void trc::rt::RayScene::addDrawable(RayTraceable obj)
{
    if (drawables.size() >= MAX_TLAS_INSTANCES) {
        throw std::runtime_error("[In RayScene::addDrawable]: Maximum drawables exceeded!");
    }

    const ui32 id = drawables.size();
    auto& d = drawables.emplace_back(
        obj,
        id,
        BLAS(instance, obj.geo.get(), blasAlloc)
    );
    modelMatrices.emplace_back(obj.modelMatrix);

    // Build BLAS
    d.blas.build();

    // Create acceleration structure instances and drawable shading data
    blasInstanceBufferMap[id] = {
        d.info.modelMatrix.get(),
        id,
        0xff,   // mask
        0,      // sbt offset
        vk::GeometryInstanceFlagBitsKHR::eForceOpaque
        | vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable,
        d.blas
    };
    drawableDataBufferMap[id] = { obj.geo.id(), obj.mat.id() };
}

auto trc::rt::RayScene::getTlas() -> TLAS&
{
    return tlas;
}

auto trc::rt::RayScene::getTlas() const -> const TLAS&
{
    return tlas;
}

auto trc::rt::RayScene::getDrawableDataBuffer() const -> vk::Buffer
{
    return *drawableDataDeviceBuffer;
}
