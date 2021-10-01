#include "drawable/RayDrawablePool.h"

#include <chrono>
#include <iostream>

#include "core/Instance.h"



trc::RayDrawablePool::RayDrawablePool(
    const ::trc::Instance& instance,
    const RayDrawablePoolCreateInfo& info)
    :
    instance(instance),
    drawables(info.maxInstances),

    // Drawable data lookup table
    drawableDataStagingBuffer(
        instance.getDevice(),
        sizeof(DrawableShadingData) * info.maxInstances,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible
    ),
    drawableDataDeviceBuffer(
        instance.getDevice(),
        sizeof(DrawableShadingData) * info.maxInstances,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    ),
    drawableDataBufferMap(reinterpret_cast<DrawableShadingData*>(drawableDataStagingBuffer.map())),

    // TLAS
    tlas(instance, info.maxInstances),
    tlasBuildDataBuffer(
        instance.getDevice(),
        sizeof(rt::GeometryInstance) * info.maxInstances,
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
        | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eHostVisible,
        vkb::DefaultDeviceMemoryAllocator{ vk::MemoryAllocateFlagBits::eDeviceAddress }
    ),
    tlasBuildDataBufferMap(reinterpret_cast<rt::GeometryInstance*>(tlasBuildDataBuffer.map())),

    // BLAS memory stuff
    blasMemoryPool(
        instance.getDevice(),
        100000000,  // 100 MB
        vk::MemoryAllocateFlagBits::eDeviceAddress
    ),
    blasAlloc(blasMemoryPool.makeAllocator())
{
    assert(instance.hasRayTracing());
}

auto trc::RayDrawablePool::buildTlas() -> std::future<void>
{
    return std::async([this] {
        instance.getDevice().executeCommandsSynchronously(
            vkb::QueueType::compute,
            [this](vk::CommandBuffer cmdBuf) {
                buildTlas(cmdBuf);
            }
        );
    });
}

void trc::RayDrawablePool::buildTlas(vk::CommandBuffer cmdBuf)
{
    std::unique_lock lock(allInstancesLock);
    ui32 numInstances = 0;
    for (i32 drawableIndex = 0; drawableIndex <= highestDrawableIndex; drawableIndex++)
    {
        auto& d = drawables.at(drawableIndex);

        for (auto& inst : d.instances)
        {
            tlasBuildDataBufferMap[numInstances] = rt::GeometryInstance(
                glm::transpose(inst.transform.get()),
                drawableIndex,
                0xff,
                0,
                vk::GeometryInstanceFlagBitsKHR::eForceOpaque,
                *d.blas
            );
            numInstances++;
        }
    }
    lock.unlock();

    // Copy drawable data to device-local memory
    drawableDataStagingBuffer.flush();
    cmdBuf.copyBuffer(
        *drawableDataStagingBuffer, *drawableDataDeviceBuffer,
        vk::BufferCopy(0, 0, sizeof(DrawableShadingData) * drawables.size())
    );

    // Build TLAS
    tlasBuildDataBuffer.flush();
    tlas.build(cmdBuf, *tlasBuildDataBuffer, numInstances);
}

void trc::RayDrawablePool::createDrawable(ui32 drawableId, const DrawableCreateInfo& info)
{
    assert(drawableId < drawables.size());
    assert(drawables.at(drawableId).instances.empty());

    highestDrawableIndex = glm::max(highestDrawableIndex, static_cast<i32>(drawableId));

    auto& d = drawables.at(drawableId);
    d.blas.reset(new rt::BLAS(instance, info.geo.get(), blasAlloc));
    d.blas->build();

    // Add data to lookup table
    drawableDataBufferMap[drawableId] = { info.geo, info.mat };
}

void trc::RayDrawablePool::deleteDrawable(ui32 drawableId)
{
    assert(drawables.at(drawableId).instances.empty());

    drawables.at(drawableId).blas.reset();
}

void trc::RayDrawablePool::createInstance(ui32 drawableId, const DrawableInstanceCreateInfo& info)
{
    auto& d = drawables.at(drawableId);

    std::scoped_lock lock(allInstancesLock);
    d.instances.emplace_back(info.transform, info.animData);
}

void trc::RayDrawablePool::deleteInstance(ui32 drawableId, ui32 instanceId)
{
    auto& d = drawables.at(drawableId);

    std::scoped_lock lock(allInstancesLock);
    std::swap(d.instances.at(instanceId), d.instances.back());
    d.instances.pop_back();
}

auto trc::RayDrawablePool::getTlas() const -> const rt::TLAS&
{
    return tlas;
}

auto trc::RayDrawablePool::getDrawableDataBuffer() const -> vk::Buffer
{
    return *drawableDataDeviceBuffer;
}
