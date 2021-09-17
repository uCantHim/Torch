#include "ray_tracing/AccelerationStructure.h"

#include <vkb/MemoryPool.h>

#include "core/Instance.h"
#include "ray_tracing/GeometryUtils.h"



auto trc::rt::internal::AccelerationStructureBase::operator*() const noexcept
    -> vk::AccelerationStructureKHR
{
    return *accelerationStructure;
}

void trc::rt::internal::AccelerationStructureBase::create(
    const ::trc::Instance& instance,
    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo,
    const vk::ArrayProxy<const ui32>& primitiveCount,
    const vkb::DeviceMemoryAllocator& alloc)
{
    geoBuildInfo = buildInfo;

    buildSizes = instance.getDevice()->getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eHostOrDevice,
        geoBuildInfo,
        // Collect number of primitives for each geometry in the build
        // info
        primitiveCount,
        instance.getDL()
    );

    /**
     * TODO: Create a buffer pool instead of one buffer per AS
     */
    accelerationStructureBuffer = vkb::DeviceLocalBuffer{
        instance.getDevice(),
        buildSizes.accelerationStructureSize, nullptr,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR,
        alloc
    };
    accelerationStructure = instance.getDevice()->createAccelerationStructureKHRUnique(
        vk::AccelerationStructureCreateInfoKHR{
            {},
            *accelerationStructureBuffer,
            0, // buffer offset
            buildSizes.accelerationStructureSize,
            geoBuildInfo.type
        },
        nullptr,
        instance.getDL()
    );
}

auto trc::rt::internal::AccelerationStructureBase::getGeometryBuildInfo() const noexcept
    -> const vk::AccelerationStructureBuildGeometryInfoKHR&
{
    return geoBuildInfo;
}

auto trc::rt::internal::AccelerationStructureBase::getBuildSize() const noexcept
    -> const vk::AccelerationStructureBuildSizesInfoKHR&
{
    return buildSizes;
}



trc::rt::BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(
    const ::trc::Instance& instance,
    Geometry geo,
    const vkb::DeviceMemoryAllocator& alloc)
    :
    BottomLevelAccelerationStructure(instance, std::vector<Geometry>{ geo }, alloc)
{
}

trc::rt::BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(
    const ::trc::Instance& instance,
    std::vector<Geometry> geos,
    const vkb::DeviceMemoryAllocator& alloc)
    :
    instance(instance),
    geometries([&] {
        std::vector<vk::AccelerationStructureGeometryKHR> result;
        for (Geometry geo : geos) {
            result.push_back(makeGeometryInfo(instance.getDevice(), geo));
        }
        return result;
    }()),
    primitiveCounts(
        [this]() -> std::vector<ui32>
        {
            std::vector<ui32> numPrimitivesPerGeo;
            for (auto& geo : geometries)
            {
                assert(geo.geometryType == vk::GeometryTypeKHR::eTriangles);
                numPrimitivesPerGeo.push_back(geo.geometry.triangles.maxVertex / 3);
            }
            return numPrimitivesPerGeo;
        }()  // array of number of primitives for each geo in geometryinfo
    )
{
    // Create the acceleration structure now (when the geometries vector
    // is initialized)
    create(
        instance,
        vk::AccelerationStructureBuildGeometryInfoKHR{
            vk::AccelerationStructureTypeKHR::eBottomLevel,
            vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            vk::BuildAccelerationStructureModeKHR::eBuild,
            {}, {}, // src and dst AS if mode is update
            static_cast<ui32>(geometries.size()),
            geometries.data()
        },
        primitiveCounts,
        alloc
    );

    deviceAddress = instance.getDevice()->getAccelerationStructureAddressKHR(
        { *accelerationStructure },
        instance.getDL()
    );
}

void trc::rt::BottomLevelAccelerationStructure::build()
{
    static auto features = instance.getPhysicalDevice().physicalDevice.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR
    >().get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();

    auto [buildRanges, buildRangePointers] = makeBuildRanges();

    // Create a temporary scratch buffer
    vkb::DeviceLocalBuffer scratchBuffer{
        instance.getDevice(),
        buildSizes.buildScratchSize, nullptr,
        vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer
    };

    // Decide whether the AS should be built on the host or on the device.
    // Build on the host if possible.
    if (features.accelerationStructureHostCommands)
    {
        instance.getDevice()->buildAccelerationStructuresKHR(
            {}, // optional deferred operation
            geoBuildInfo
                .setScratchData(instance.getDevice()->getBufferAddress({ *scratchBuffer }))
                .setDstAccelerationStructure(*accelerationStructure),
            buildRangePointers,
            instance.getDL()
        );
    }
    else
    {
        instance.getDevice().executeCommandsSynchronously(
            vkb::QueueType::compute,
            [&, &buildRangePointers=buildRangePointers](vk::CommandBuffer cmdBuf)
            {
                cmdBuf.buildAccelerationStructuresKHR(
                    geoBuildInfo
                        .setScratchData(instance.getDevice()->getBufferAddress({ *scratchBuffer }))
                        .setDstAccelerationStructure(*accelerationStructure),
                    buildRangePointers,
                    instance.getDL()
                );
            }
        );
    }
}

auto trc::rt::BottomLevelAccelerationStructure::getDeviceAddress() const noexcept -> ui64
{
    return deviceAddress;
}

auto trc::rt::BottomLevelAccelerationStructure::makeBuildRanges() const noexcept
    -> std::pair<
        std::unique_ptr<std::vector<vk::AccelerationStructureBuildRangeInfoKHR>>,
        std::vector<const vk::AccelerationStructureBuildRangeInfoKHR*>
    >
{
    std::vector<const vk::AccelerationStructureBuildRangeInfoKHR*> buildRangePointers;
    auto buildRanges = [this, &buildRangePointers] {
        auto result = std::make_unique<std::vector<vk::AccelerationStructureBuildRangeInfoKHR>>();
        result->reserve(primitiveCounts.size());
        for (ui32 numPrimitives : primitiveCounts)
        {
            buildRangePointers.push_back(&result->emplace_back(
                numPrimitives, // primitive count
                0, // primitive offset
                0, // first vertex index
                0  // transform offset
            ));
        }
        return result;
    }();

    return { std::move(buildRanges), std::move(buildRangePointers) };
}



trc::rt::TopLevelAccelerationStructure::TopLevelAccelerationStructure(
    const ::trc::Instance& instance)
    :
    instance(instance)
{
}

trc::rt::TopLevelAccelerationStructure::TopLevelAccelerationStructure(
    const ::trc::Instance& instance,
    ui32 maxInstances,
    const vkb::DeviceMemoryAllocator& alloc)
    :
    instance(instance),
    maxInstances(maxInstances),
    geometry(
        vk::GeometryTypeKHR::eInstances,
        vk::AccelerationStructureGeometryDataKHR{
            vk::AccelerationStructureGeometryInstancesDataKHR{
                false, // Not an array of pointers
                nullptr
            }
        }
    )
{
    create(
        instance,
        vk::AccelerationStructureBuildGeometryInfoKHR{
            vk::AccelerationStructureTypeKHR::eTopLevel,
            vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            vk::BuildAccelerationStructureModeKHR::eBuild,
            {}, {}, // src and dst AS if mode is update
            1, // geometryCount MUST be 1 for top-level AS
            &geometry
        },
        maxInstances,
        alloc
    );
}

void trc::rt::TopLevelAccelerationStructure::build(
    vk::Buffer instanceBuffer,
    ui32 offset)
{
    // Use new instance buffer
    geometry.geometry.instances.data = instance.getDevice()->getBufferAddress(instanceBuffer);

    // Create temporary scratch buffer
    vkb::DeviceLocalBuffer scratchBuffer{
        instance.getDevice(),
        buildSizes.buildScratchSize,
        nullptr,
        vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer
    };

    vk::AccelerationStructureBuildRangeInfoKHR buildRange{ maxInstances, offset, 0, 0 };
    instance.getDevice().executeCommandsSynchronously(vkb::QueueType::compute,
        [&](vk::CommandBuffer cmdBuf)
        {
            cmdBuf.buildAccelerationStructuresKHR(
                geoBuildInfo
                    .setGeometries(geometry)
                    .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
                    .setDstAccelerationStructure(*accelerationStructure)
                    .setScratchData(instance.getDevice()->getBufferAddress({ *scratchBuffer })),
                { &buildRange },
                instance.getDL()
            );
        }
    );
}



// ---------------- //
//      Helpers     //
// ---------------- //

void trc::rt::buildAccelerationStructures(
    const ::trc::Instance& instance,
    const std::vector<BottomLevelAccelerationStructure*>& as)
{
    if (as.empty()) return;

    static auto features = instance.getPhysicalDevice().physicalDevice.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR
    >().get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();

    // Create a temporary scratch buffer
    const ui32 scratchSize = [&as] {
        ui32 result{ 0 };
        for (auto& blas : as) {
            result += blas->getBuildSize().buildScratchSize;
        }
        return result;
    }();
    vkb::MemoryPool scratchPool(instance.getDevice(), scratchSize);
    std::vector<vkb::DeviceLocalBuffer> scratchBuffers;

    // Collect build infos
    std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> geoBuildInfos;
    for (auto& blas : as)
    {
        vkb::DeviceLocalBuffer& scratchBuffer = scratchBuffers.emplace_back(
            instance.getDevice(),
            blas->getBuildSize().buildScratchSize, nullptr,
            vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer,
            scratchPool.makeAllocator()
        );
        auto info = blas->getGeometryBuildInfo();
        geoBuildInfos.push_back(
            info.setScratchData(instance.getDevice()->getBufferAddress({ *scratchBuffer }))
                .setDstAccelerationStructure(**blas)
        );
    }

    geoBuildInfos[0].pGeometries[0].geometry.triangles.vertexData;

    // Collect build ranges for all acceleration structures
    std::vector<const vk::AccelerationStructureBuildRangeInfoKHR*> buildRangePointers;
    std::vector<std::unique_ptr<std::vector<vk::AccelerationStructureBuildRangeInfoKHR>>> buildRanges;
    for (auto& blas : as)
    {
        auto [ranges, ptr] = blas->makeBuildRanges();
        buildRangePointers.insert(buildRangePointers.end(), ptr.begin(), ptr.end());
        buildRanges.push_back(std::move(ranges));
    }

    // Decide whether the AS should be built on the host or on the device.
    // Build on the host if possible.
    if (features.accelerationStructureHostCommands)
    {
        instance.getDevice()->buildAccelerationStructuresKHR(
            {}, // optional deferred operation
            geoBuildInfos,
            buildRangePointers,
            instance.getDL()
        );
    }
    else
    {
        instance.getDevice().executeCommandsSynchronously(
            vkb::QueueType::compute,
            [&](vk::CommandBuffer cmdBuf)
            {
                cmdBuf.buildAccelerationStructuresKHR(
                    geoBuildInfos,
                    buildRangePointers,
                    instance.getDL()
                );
            }
        );
    }
}
