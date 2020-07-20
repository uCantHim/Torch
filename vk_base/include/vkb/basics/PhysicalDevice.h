#pragma once

#include <vector>
#include <array>

#include <vulkan/vulkan.hpp>

namespace vkb
{
    namespace phys_device_properties
    {
        /* A queue type is the capability of a queue to perform a certain task.
        Queue types are defined by the queue family that a queue belongs to.
        Queues can have multiple queue types. */
        enum class QueueType {
            graphics, compute,
            transfer,
            sparseMemory, protectedMemory,
            presentation,
            numQueueTypes
        };

        using familyIndex = uint32_t;

        struct QueueFamily {
            QueueFamily() = default;
            QueueFamily(uint32_t _index, uint32_t _queueCount,
                        std::array<bool, static_cast<size_t>(QueueType::numQueueTypes)> capabilities)
                :
                index(_index),
                queueCount(_queueCount),
                capabilities(capabilities.begin(), capabilities.end())
            {}

            familyIndex index;
            uint32_t queueCount;

            bool isCapable(QueueType type) const noexcept {
                return capabilities[static_cast<size_t>(type)];
            }

        private:
            std::vector<bool> capabilities;
        };

        struct QueueFamilies {
            std::vector<QueueFamily> graphicsFamilies;
            std::vector<QueueFamily> computeFamilies;
            std::vector<QueueFamily> transferFamilies;
            std::vector<QueueFamily> sparseMemoryFamilies;
            std::vector<QueueFamily> protectedMemoryFamilies;
            std::vector<QueueFamily> presentationFamilies;
        };

        struct SwapchainSupport {
            vk::SurfaceCapabilitiesKHR surfaceCapabilities;
            std::vector<vk::SurfaceFormatKHR> surfaceFormats;
            std::vector<vk::PresentModeKHR> surfacePresentModes;
        };

        auto findSupportedQueueFamilies(
            const vk::PhysicalDevice& device,
            const vk::SurfaceKHR& surface
        ) -> QueueFamilies;

        auto findSwapchainSupport(
            const vk::PhysicalDevice& device,
            const vk::SurfaceKHR& surface
        ) -> SwapchainSupport;
    } // namespace phys_device_properties


    /**
     * @brief A physical vulkan-capable device
     */
    class PhysicalDevice
    {
    public:
        /**
         * @brief Create a physical device object
         *
         * Requires a surface because it has to query for swapchain and
         * presentation support.
         */
        PhysicalDevice(vk::PhysicalDevice device, vk::SurfaceKHR surface);

        PhysicalDevice(const PhysicalDevice&) = default;
        PhysicalDevice(PhysicalDevice&&) noexcept = default;
        ~PhysicalDevice() = default;

        PhysicalDevice& operator=(const PhysicalDevice&) = delete;
        PhysicalDevice& operator=(PhysicalDevice&&) noexcept = delete;

        /**
         * @brief Create a logical device from the physical device
         */
        auto createLogicalDevice() const -> vk::UniqueDevice;

        /**
         * @return A list of all existing queue families on the device
         */
        auto getUniqueQueueFamilies() const noexcept
            -> std::vector<phys_device_properties::QueueFamily>;

        auto getSwapchainSupport(const vk::SurfaceKHR& surface) const noexcept
            -> phys_device_properties::SwapchainSupport;

        /**
         * @brief Find the index of a memory type with specific properties
         *
         * @param uint32_t memoryTypeBits The memory type bits in a queried
         *                                vk::MemoryRequirements structure
         * @param vk::MemoryPropertyFlags properties Required memory
         *                                           properties
         *
         * @return uint32_t The index of the searched memory type
         */
        uint32_t findMemoryType(uint32_t memoryTypeBits, vk::MemoryPropertyFlags properties) const;


        ///////////////////////////
        // Public device properties

        vk::PhysicalDevice physicalDevice;
        std::string name;
        vk::PhysicalDeviceType type;
        std::string typeString;

        const phys_device_properties::QueueFamilies queueFamilies;
        const std::vector<vk::ExtensionProperties> supportedExtensions;
        const vk::PhysicalDeviceProperties properties;
        const vk::PhysicalDeviceFeatures features;

        const vk::PhysicalDeviceMemoryProperties memoryProperties;
    };

    namespace device_helpers
    {
        auto findAllPhysicalDevices(vk::Instance instance, vk::SurfaceKHR surface)
            -> std::vector<PhysicalDevice>;

        auto getOptimalPhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface)
            -> PhysicalDevice;

        bool isOptimalDevice(const PhysicalDevice& device);

        /**
         * Required queue families are:
         *  - graphics family
         *  - presentation family
         *  - transfer family
         */
        bool supportsRequiredQueueFamilies(const PhysicalDevice& device);
        bool supportsRequiredDeviceExtensions(const PhysicalDevice& device);
    }
} // namespace vkb
