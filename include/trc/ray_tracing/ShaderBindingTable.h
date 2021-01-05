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
            std::vector<ui32> entrySizes,
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

        struct GroupEntry
        {
            vkb::DeviceLocalBuffer buffer;
            vk::StridedDeviceAddressRegionKHR address;
        };

        std::vector<GroupEntry> entries;

        std::unordered_map<std::string, ui32> shaderGroupAliases;
    };
} // namespace trc::rt
