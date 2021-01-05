#include "ray_tracing/ShaderBindingTable.h"

#include <numeric>

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
    std::vector<ui32> entrySizes,
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
    const ui32 numShaderGroups = std::accumulate(entrySizes.begin(), entrySizes.end(), 0u);
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

    // Calculate shader group addresses
    for (ui32 i = 0; i < numShaderGroups; i++)
    {
        const ui32 stride = alignedGroupHandleSize * (i + 1);
        if (stride > rayTracingProperties.maxShaderGroupStride)
        {
            throw std::runtime_error(
                "A shader group stride of " + std::to_string(stride)
                + " exceeds the hardware's limit of "
                + std::to_string(rayTracingProperties.maxShaderGroupStride)
            );
        }

        vkb::DeviceLocalBuffer buffer{
            device,
            alignedGroupHandleSize,
            shaderHandleStorage.data() + groupHandleSize * i,
            vk::BufferUsageFlagBits::eShaderBindingTableKHR
            | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            alloc
        };
        vk::StridedDeviceAddressRegionKHR address(
            device->getBufferAddress({ *buffer }),
            stride,              // stride
            alignedGroupHandleSize  // size
        );
        entries.emplace_back(std::move(buffer), address);
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
    return entries.at(shaderGroupIndex).address;
}

auto trc::rt::ShaderBindingTable::getShaderGroupAddress(const std::string& shaderGroupName)
    -> vk::StridedDeviceAddressRegionKHR
{
    return getShaderGroupAddress(shaderGroupAliases.at(shaderGroupName));
}
