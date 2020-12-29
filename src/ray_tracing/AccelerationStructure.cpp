#include "ray_tracing/AccelerationStructure.h"



auto trc::rt::internal::AccelerationStructureBase::operator*() const noexcept
    -> vk::AccelerationStructureKHR
{
    return *accelerationStructure;
}

void trc::rt::internal::AccelerationStructureBase::create(
    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo,
    const vk::ArrayProxy<const ui32>& primitiveCount)
{
    geoBuildInfo = buildInfo;

    buildSizes = vkb::getDevice()->getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eHostOrDevice,
        geoBuildInfo,
        // Collect number of primitives for each geometry in the build
        // info
        primitiveCount,
        vkb::getDL()
    );

    /**
     * TODO: Create a buffer pool instead of one buffer per AS
     */
    accelerationStructureBuffer = vkb::DeviceLocalBuffer{
        buildSizes.accelerationStructureSize, nullptr,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR
    };
    accelerationStructure = vkb::getDevice()->createAccelerationStructureKHRUnique(
        vk::AccelerationStructureCreateInfoKHR{
            {},
            *accelerationStructureBuffer,
            0, // buffer offset
            buildSizes.accelerationStructureSize,
            geoBuildInfo.type
        },
        nullptr,
        vkb::getDL()
    );
}



trc::rt::BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(GeometryID geo)
    :
    BottomLevelAccelerationStructure(std::vector<GeometryID>{ geo })
{
}

trc::rt::BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(
    std::vector<GeometryID> geos)
    :
    geometries([&] {
        std::vector<vk::AccelerationStructureGeometryKHR> result;
        for (GeometryID geo : geos) {
            result.push_back(makeGeometryInfo(geo));
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
        vk::AccelerationStructureBuildGeometryInfoKHR{
            vk::AccelerationStructureTypeKHR::eBottomLevel,
            vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            vk::BuildAccelerationStructureModeKHR::eBuild,
            {}, {}, // src and dst AS if mode is update
            static_cast<ui32>(geometries.size()),
            geometries.data()
        },
        primitiveCounts
    );

    deviceAddress = vkb::getDevice()->getAccelerationStructureAddressKHR(
        { *accelerationStructure },
        vkb::getDL()
    );
}

void trc::rt::BottomLevelAccelerationStructure::build()
{
    static auto features = vkb::getPhysicalDevice().physicalDevice.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR
    >().get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();

    // Build ranges, the interface for these is terrible
    std::vector<const vk::AccelerationStructureBuildRangeInfoKHR*> buildRangePointers;
    const auto buildRanges = [this, &buildRangePointers] {
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR> result;
        for (ui32 numPrimitives : primitiveCounts)
        {
            buildRangePointers.push_back(&result.emplace_back(
                numPrimitives, // primitive count
                0, // primitive offset
                0, // first vertex index
                0  // transform offset
            ));
        }
        return result;
    }();

    // Create a temporary scratch buffer
    vkb::DeviceLocalBuffer scratchBuffer{
        buildSizes.buildScratchSize, nullptr,
        vk::BufferUsageFlagBits::eShaderDeviceAddress
    };

    // Decide whether the AS should be built on the host or on the device.
    // Build on the host if possible.
    if (features.accelerationStructureHostCommands)
    {
        vkb::getDevice()->buildAccelerationStructuresKHR(
            {}, // optional deferred operation
            geoBuildInfo
                .setScratchData(vkb::getDevice()->getBufferAddress({ *scratchBuffer }))
                .setDstAccelerationStructure(*accelerationStructure),
            buildRangePointers,
            vkb::getDL()
        );
    }
    else
    {
        vkb::getDevice().executeCommandsSynchronously(
            vkb::QueueType::compute,
            [&](vk::CommandBuffer cmdBuf)
            {
                cmdBuf.buildAccelerationStructuresKHR(
                    geoBuildInfo
                        .setScratchData(vkb::getDevice()->getBufferAddress({ *scratchBuffer }))
                        .setDstAccelerationStructure(*accelerationStructure),
                    buildRangePointers,
                    vkb::getDL()
                );
            }
        );
    }
}

auto trc::rt::BottomLevelAccelerationStructure::getDeviceAddress() const noexcept -> ui64
{
    return deviceAddress;
}



trc::rt::TopLevelAccelerationStructure::TopLevelAccelerationStructure(uint32_t maxInstances)
    :
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
        vk::AccelerationStructureBuildGeometryInfoKHR{
            vk::AccelerationStructureTypeKHR::eTopLevel,
            vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            vk::BuildAccelerationStructureModeKHR::eBuild,
            {}, {}, // src and dst AS if mode is update
            1, // geometryCount MUST be 1 for top-level AS
            &geometry
        },
        maxInstances
    );
}

void trc::rt::TopLevelAccelerationStructure::build(
    const vkb::Buffer& instanceBuffer,
    ui32 offset)
{
    // Use new instance buffer
    geometry.geometry.instances.data = vkb::getDevice()->getBufferAddress(*instanceBuffer);

    // Create temporary scratch buffer
    vkb::DeviceLocalBuffer scratchBuffer{
        buildSizes.buildScratchSize,
        nullptr,
        vk::BufferUsageFlagBits::eShaderDeviceAddress
    };

    vk::AccelerationStructureBuildRangeInfoKHR buildRange{ maxInstances, offset, 0, 0 };
    vkb::getDevice().executeCommandsSynchronously(vkb::QueueType::compute,
        [&](vk::CommandBuffer cmdBuf)
        {
            cmdBuf.buildAccelerationStructuresKHR(
                geoBuildInfo
                    .setDstAccelerationStructure(*accelerationStructure)
                    .setScratchData(vkb::getDevice()->getBufferAddress({ *scratchBuffer })),
                { &buildRange },
                vkb::getDL()
            );
        }
    );
}



// ---------------- //
//      Helpers     //
// ---------------- //

//void trc::rt::buildAccelerationStructures(const std::vector<BottomLevelAccelerationStructure>& as)
//{
//    if (as.empty()) {
//        return;
//    }
//
//    auto scratchMemReq = Globals::getDevice().getAccelerationStructureMemoryRequirementsNV(
//        { vk::AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch, *as[0] },
//        Globals::getFuncLoader()
//    ).memoryRequirements;
//
//    const vk::DeviceSize scratchBufferOffset = scratchMemReq.size;
//    const vk::DeviceSize maxScratchMemory = scratchMemReq.size * as.size();
//
//    Buffer scratchBuffer(maxScratchMemory,
//                         vk::BufferUsageFlagBits::eRayTracingNV,
//                         vk::MemoryPropertyFlagBits::eDeviceLocal);
//
//    auto cmdBuf = Globals::makeCmdBuf();
//    cmdBuf->begin({});
//    for (size_t i = 0; i < as.size(); i++)
//    {
//        const auto& structure = as[i];
//
//        cmdBuf->buildAccelerationStructureNV(
//            structure.getInfo(),
//            {}, 0, // instance data and offset
//            VK_FALSE,
//            *structure, {}, // dst, src
//            *scratchBuffer, scratchBufferOffset * i,
//            Globals::getFuncLoader());
//    }
//    cmdBuf->end();
//
//    util::executeCommandBuffer(*cmdBuf);
//}
