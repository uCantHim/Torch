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
        /**
         * @brief Create a shader binding table
         *
         * Is is advised to pass an allocator to this constructor that
         * allocates from a memory pool. Otherwise, each entry in the table
         * gets its own allociation.
         *
         * @param const Device& device
         * @param vk::Pipeline pipeline The ray tracing pipeline that this
         *                              table refers to
         * @param std::vector<ui32> entrySizes One ui32 for each entry in
         *        the table. The number signals how many subsequent shader
         *        groups are present in the entry.
         * @param const vkb::DeviceMemoryAllocator& alloc
         */
        ShaderBindingTable(
            const vkb::Device& device,
            vk::Pipeline pipeline,
            std::vector<ui32> entrySizes,
            const vkb::DeviceMemoryAllocator& alloc = vkb::DefaultDeviceMemoryAllocator{}
        );

        /**
         * @brief Map an entry index to a string
         */
        void setEntryAlias(std::string entryName, ui32 entryIndex);

        /**
         * @return vk::StridedDeviceAddressRegionKHR The address of the
         *         entryIndex-th entry in the shader binding table.
         */
        auto getEntryAddress(ui32 entryIndex) -> vk::StridedDeviceAddressRegionKHR;

        /**
         * @return vk::StridedDeviceAddressRegionKHR The address of an
         *         entry with name entryName.
         * @throw std::out_of_range if no entry has previously been mapped
         *        to entryName.
         */
        auto getEntryAddress(const std::string& entryName)
            -> vk::StridedDeviceAddressRegionKHR;

    private:
        // For shader group handle alignment
        static inline vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties;
        static vkb::StaticInit _init;

        /**
         * A single entry in the shader binding table has its own buffer
         * and an address region specifying the address of the entry.
         */
        struct GroupEntry
        {
            vkb::DeviceLocalBuffer buffer;
            vk::StridedDeviceAddressRegionKHR address;
        };

        std::vector<GroupEntry> entries;
        std::unordered_map<std::string, ui32> entryAliases;
    };
} // namespace trc::rt
