#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <vkb/Buffer.h>
#include <vkb/StaticInit.h>

#include "Types.h"

namespace trc::rt
{
    class ShaderBindingTable
    {
    public:
        ShaderBindingTable(
            const vkb::Device& device,
            vk::Pipeline pipeline,
            ui32 numShaderGroups,
            const vkb::DeviceMemoryAllocator& alloc = vkb::DefaultDeviceMemoryAllocator{}
        );

        /**
         * @brief Map a shader group index to a string
         */
        void setGroupAlias(std::string shaderGroupName, ui32 shaderGroupIndex);

        auto getShaderGroupAddress(ui32 shaderGroupIndex) -> vk::StridedDeviceAddressRegionKHR;
        auto getShaderGroupAddress(const std::string& shaderGroupName)
            -> vk::StridedDeviceAddressRegionKHR;

    private:
        static inline vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties;
        static vkb::StaticInit _init;

        vkb::DeviceLocalBuffer buffer;
        vk::DeviceAddress bufferDeviceAddress;
        std::vector<vk::StridedDeviceAddressRegionKHR> groupAddresses;

        std::unordered_map<std::string, ui32> shaderGroupAliases;
    };
} // namespace trc::rt
