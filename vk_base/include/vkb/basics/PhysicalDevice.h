#pragma once

#include <vector>
#include <array>

#include <vulkan/vulkan.hpp>

#include "VulkanDebug.h"

namespace vkb
{
    /* A queue type is the capability of a queue to perform a certain task.
    Queue types are defined by the queue family that a queue belongs to.
    Queues can have multiple queue types. */
    enum class QueueType
    {
        graphics, compute,
        transfer,
        sparseMemory, protectedMemory,
        presentation,
        numQueueTypes
    };

    using QueueFamilyIndex = uint32_t;

    struct QueueFamily
    {
        QueueFamily() = default;
        QueueFamily(uint32_t _index, uint32_t _queueCount,
                    std::array<bool, static_cast<size_t>(QueueType::numQueueTypes)> capabilities)
            :
            index(_index),
            queueCount(_queueCount),
            capabilities(capabilities.begin(), capabilities.end())
        {}

        QueueFamilyIndex index;
        uint32_t queueCount;

        bool isCapable(QueueType type) const noexcept {
            return capabilities[static_cast<size_t>(type)];
        }

    private:
        std::vector<bool> capabilities;
    };

    struct QueueFamilyCapabilities
    {
        std::vector<QueueFamily> graphicsCapable;
        std::vector<QueueFamily> computeCapable;
        std::vector<QueueFamily> transferCapable;
        std::vector<QueueFamily> sparseMemoryCapable;
        std::vector<QueueFamily> protectedMemoryCapable;
        std::vector<QueueFamily> presentationCapable;
    };

    extern auto getQueueFamilies(vk::PhysicalDevice, vk::SurfaceKHR surface)
        -> std::vector<QueueFamily>;
    extern auto sortByCapabilities(const std::vector<QueueFamily>& families)
        -> QueueFamilyCapabilities;


    /**
     * @brief A physical vulkan-capable device
     */
    class PhysicalDevice
    {
    public:
        struct SwapchainSupport
        {
            vk::SurfaceCapabilitiesKHR surfaceCapabilities;
            std::vector<vk::SurfaceFormatKHR> surfaceFormats;
            std::vector<vk::PresentModeKHR> surfacePresentModes;
        };

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
         *
         * Some basic extensions that are required to be supported by the
         * specification will be enabled by default.
         *
         * @param std::vector<const char*> deviceExtensions Extensions to
         *        enable on the device.
         */
        template<typename... DeviceFeatures>
        auto createLogicalDevice(std::vector<const char*> deviceExtensions = {}) const
            -> vk::UniqueDevice;

        auto getSwapchainSupport(vk::SurfaceKHR surface) const noexcept
            -> SwapchainSupport;

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

        const vk::PhysicalDevice physicalDevice;

        // Queues
        const std::vector<QueueFamily> queueFamilies;
        const QueueFamilyCapabilities queueFamilyCapabilities;

        // Physical device properties
        const std::vector<vk::ExtensionProperties> supportedExtensions;
        const vk::PhysicalDeviceProperties properties;
        const vk::PhysicalDeviceFeatures features;

        // Memory
        const vk::PhysicalDeviceMemoryProperties memoryProperties;

        // Other
        const std::string name;
        const vk::PhysicalDeviceType type;
        const std::string typeString;
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
        bool supportsRequiredQueueCapabilities(const PhysicalDevice& device);
        bool supportsRequiredDeviceExtensions(const PhysicalDevice& device);

        /**
         * @brief Basic extensions that are always loaded.
         */
        auto getRequiredDeviceExtensions() -> std::vector<const char*>;
    }



    template<typename... DeviceFeatures>
    auto vkb::PhysicalDevice::createLogicalDevice(std::vector<const char*> deviceExtensions) const
        -> vk::UniqueDevice
    {
        // Device queues
        std::vector<float> prios(100, 1.0f); // Enough prios for 100 queues per family
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        for (const auto& queueFamily : queueFamilies)
        {
            queueCreateInfos.push_back(
                { {}, queueFamily.index, queueFamily.queueCount, prios.data() }
            );
        }

        // Validation layers
        const auto validationLayers = getRequiredValidationLayers();

        // Extensions
        const auto requiredDevExt = device_helpers::getRequiredDeviceExtensions();
        deviceExtensions.insert(deviceExtensions.end(), requiredDevExt.begin(), requiredDevExt.end());

        // Device features
        auto deviceFeatures = physicalDevice.getFeatures2<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceDescriptorIndexingFeatures,
            DeviceFeatures...
        >();

        // Create the logical device
        vk::StructureChain chain
        {
            vk::DeviceCreateInfo(
                {},
                static_cast<uint32_t>(queueCreateInfos.size()), queueCreateInfos.data(),
                static_cast<uint32_t>(validationLayers.size()), validationLayers.data(),
                static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data(),
                &deviceFeatures.template get<vk::PhysicalDeviceFeatures2>().features
            ),
            // This must be the first feature in the structure chain. This
            // works because descriptor indexing is always enabled.
            deviceFeatures.template get<vk::PhysicalDeviceDescriptorIndexingFeatures>(),
        };

        return physicalDevice.createDeviceUnique(chain.template get<vk::DeviceCreateInfo>());
    }
} // namespace vkb


namespace std
{
    extern auto to_string(vkb::QueueType queueType) -> std::string;
}
