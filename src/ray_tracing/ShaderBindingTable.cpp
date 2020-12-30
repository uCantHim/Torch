#include "ray_tracing/ShaderBindingTable.h"

#include "utils/Util.h"



vkb::StaticInit trc::rt::ShaderBindingTable::_init{
    [] {
        rayTracingProperties = vkb::getPhysicalDevice().physicalDevice.getProperties2<
            vk::PhysicalDeviceProperties2,
            vk::PhysicalDeviceRayTracingPipelinePropertiesKHR
        >().get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    }
};

trc::rt::ShaderBindingTable::ShaderBindingTable(
    const vkb::Device& device,
    vk::Pipeline pipeline,
    ui32 numShaderGroups,
    const vkb::DeviceMemoryAllocator& alloc)
{
    const auto rayTracingProperties = device.getPhysicalDevice().physicalDevice.getProperties2<
        vk::PhysicalDeviceProperties2,
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR
    >().get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

    const ui32 groupHandleSize = rayTracingProperties.shaderGroupHandleSize;
    const ui32 alignedGroupHandleSize = util::pad(
        groupHandleSize, rayTracingProperties.shaderGroupBaseAlignment
    );
    const vk::DeviceSize sbtSize = alignedGroupHandleSize * numShaderGroups;

    std::vector<ui8> shaderHandleStorage(sbtSize);
    auto result = vkb::getDevice()->getRayTracingShaderGroupHandlesKHR(
        pipeline,
        0, // first group
        numShaderGroups, // group count
        shaderHandleStorage.size(),
        shaderHandleStorage.data(),
        vkb::getDL()
    );
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Unable to retrieve shader group handles");
    }

    // Copy retrieved group handles into aligned storage because the handles
    // from vkGetRayTracingShaderGroupHandles are tightly packed. The Vulkan
    // spec requires group offsets in vkCmdTraceRaysNV to be a multiple of
    // VkPhysicalDeviceRayTracingPropertiesNV::shaderGroupHandleBaseAlignment.
    std::vector<ui8> alignedStorage(sbtSize);
    for (size_t i = 0; i < numShaderGroups; i++)
    {
        memcpy(
            &alignedStorage[i * alignedGroupHandleSize],
            &shaderHandleStorage[i * groupHandleSize],
            groupHandleSize
        );
    }

    buffer = vkb::DeviceLocalBuffer{
        device,
        alignedStorage,
        vk::BufferUsageFlagBits::eShaderBindingTableKHR
        | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        alloc
    };
    bufferDeviceAddress = device->getBufferAddress({ *buffer });

    // Calculate shader group addresses
    for (ui32 i = 0; i < numShaderGroups; i++)
    {
        const ui32 stride = groupHandleSize * i;
        if (stride > rayTracingProperties.maxShaderGroupStride)
        {
            throw std::runtime_error(
                "A shader group stride of " + std::to_string(stride)
                + " exceeds the hardware's limit of "
                + std::to_string(rayTracingProperties.maxShaderGroupStride)
            );
        }

        groupAddresses.emplace_back(
            bufferDeviceAddress,
            stride,              // stride
            groupHandleSize * i  // size
        );
    }
}

void trc::rt::ShaderBindingTable::setGroupAlias(
    std::string shaderGroupName,
    ui32 shaderGroupIndex)
{
    shaderGroupAliases.try_emplace(std::move(shaderGroupName), shaderGroupIndex);
}

auto trc::rt::ShaderBindingTable::getShaderGroupAddress(ui32 shaderGroupIndex)
    -> vk::StridedDeviceAddressRegionKHR
{
    return groupAddresses.at(shaderGroupIndex);
}

auto trc::rt::ShaderBindingTable::getShaderGroupAddress(const std::string& shaderGroupName)
    -> vk::StridedDeviceAddressRegionKHR
{
    return getShaderGroupAddress(shaderGroupAliases.at(shaderGroupName));
}
